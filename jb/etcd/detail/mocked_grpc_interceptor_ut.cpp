#include "jb/etcd/detail/mocked_grpc_interceptor.hpp"
#include <jb/etcd/completion_queue.hpp>
#include <jb/gmock/init.hpp>
#include <jb/testing/future_status.hpp>

#include <etcd/etcdserver/etcdserverpb/rpc.grpc.pb.h>

#include <boost/test/unit_test.hpp>
#include <atomic>
#include <thread>

/**
 * @test Make sure we can mock async_rpc() calls a completion_queue.
 */
BOOST_AUTO_TEST_CASE(mocked_grpc_interceptor_rpc) {
  using namespace std::chrono_literals;

  // Create a null lease object, we do not need (or want) a real
  // connection for mocked operations ...
  std::shared_ptr<etcdserverpb::Lease::Stub> lease;

  using namespace jb::etcd;
  completion_queue<detail::mocked_grpc_interceptor> queue;

  // Prepare the Mock to save the asynchronous operation state,
  // normally you would simply invoke the callback in the mock action,
  // but this test wants to verify what happens if there is a delay
  // ...
  using ::testing::_;
  using ::testing::Invoke;
  std::shared_ptr<jb::etcd::detail::base_async_op> last_op;
  EXPECT_CALL(*queue.interceptor().shared_mock, async_rpc(_))
      .WillRepeatedly(
          Invoke([&last_op](auto const& op) mutable { last_op = op; }));

  // ... make the request, that will post operations to the mock
  // completion queue ...
  etcdserverpb::LeaseGrantRequest req;
  req.set_ttl(5); // in seconds
  req.set_id(0);  // let the server pick the lease_id
  auto fut = queue.async_rpc(
      lease.get(), &etcdserverpb::Lease::Stub::AsyncLeaseGrant, std::move(req),
      "test/Lease/future", jb::etcd::use_future());

  // ... verify the results are not there, the interceptor should have
  // stopped the call from going out ...
  auto wait_response = fut.wait_for(10ms);
  BOOST_CHECK_EQUAL(wait_response, std::future_status::timeout);

  // ... we need to fill the response parameters, which again could be
  // done in the mock action, but we are delaying the operations to
  // verify the std::promise is not immediately satisfied ...
  BOOST_REQUIRE(!!last_op);
  {
    auto op = dynamic_cast<jb::etcd::detail::async_op<
        etcdserverpb::LeaseGrantRequest, etcdserverpb::LeaseGrantResponse>*>(
        last_op.get());
    BOOST_CHECK(op != nullptr);
    op->response.set_ttl(7);
    op->response.set_id(123456UL);
  }
  // ... now we can execute the callback ...
  last_op->callback(*last_op, true);

  // ... that must make the result read or we will get a deadlock ...
  wait_response = fut.wait_for(10ms);
  BOOST_REQUIRE_EQUAL(wait_response, std::future_status::ready);

  // ... get the response ...
  auto response = fut.get();
  BOOST_CHECK_EQUAL(response.ttl(), 7);
  BOOST_CHECK_EQUAL(response.id(), 123456UL);
}

/**
 * @test Verify canceled RPCs result in exception for the std::promise.
 */
BOOST_AUTO_TEST_CASE(mocked_grpc_interceptor_rpc_cancelled) {
  using namespace std::chrono_literals;

  // Create a null lease object, we do not need (or want) a real
  // connection for mocked operations ...
  std::shared_ptr<etcdserverpb::Lease::Stub> lease;

  using namespace jb::etcd;
  completion_queue<detail::mocked_grpc_interceptor> queue;

  // Prepare the Mock to save the asynchronous operation state,
  // normally you would simply invoke the callback in the mock action,
  // but this test wants to verify what happens if there is a delay
  // ...
  using ::testing::_;
  using ::testing::Invoke;
  EXPECT_CALL(*queue.interceptor().shared_mock, async_rpc(_))
      .WillRepeatedly(
          Invoke([](auto bop) mutable { bop->callback(*bop, false); }));

  // ... make the request, that will post operations to the mock
  // completion queue ...
  etcdserverpb::LeaseGrantRequest req;
  req.set_ttl(5); // in seconds
  req.set_id(0);  // let the server pick the lease_id
  auto fut = queue.async_rpc(
      lease.get(), &etcdserverpb::Lease::Stub::AsyncLeaseGrant, std::move(req),
      "test/Lease/future/cancelled", jb::etcd::use_future());

  // ... check that the operation was immediately cancelled ...
  BOOST_REQUIRE_EQUAL(fut.wait_for(0ms), std::future_status::ready);

  // ... and the promise was satisfied with an exception ...
  BOOST_CHECK_THROW(fut.get(), std::exception);
}

/**
 * @test Verify creation of rdwr RPC streams is intercepted.
 */
BOOST_AUTO_TEST_CASE(mocked_grpc_interceptor_create_rdwr_stream_future) {
  using namespace std::chrono_literals;

  // Create a null lease object, we do not need (or want) a real
  // connection for mocked operations ...
  std::shared_ptr<etcdserverpb::Lease::Stub> lease;

  using namespace jb::etcd;
  completion_queue<detail::mocked_grpc_interceptor> queue;

  // Prepare the Mock to save the asynchronous operation state,
  // normally you would simply invoke the callback in the mock action,
  // but this test wants to verify what happens if there is a delay
  // ...
  using ::testing::_;
  using ::testing::Invoke;
  EXPECT_CALL(*queue.interceptor().shared_mock, async_create_rdwr_stream(_))
      .WillRepeatedly(Invoke([](auto op) mutable { op->callback(*op, true); }));

  // ... make the request, that will post operations to the mock
  // completion queue ...
  auto fut = queue.async_create_rdwr_stream(
      lease.get(), &etcdserverpb::Lease::Stub::AsyncLeaseKeepAlive,
      "test/CreateLeaseKeepAlive/future", jb::etcd::use_future());

  // ... check that the promise was immediately satisfied ...
  BOOST_REQUIRE_EQUAL(fut.wait_for(0ms), std::future_status::ready);

  // ... and that it did not raise an exception ..
  BOOST_CHECK_NO_THROW(fut.get());
  // ... we do not check the value because that is too complicated to
  // setup ...

  // ... change the mock to start canceling operations ...
  EXPECT_CALL(*queue.interceptor().shared_mock, async_create_rdwr_stream(_))
      .WillRepeatedly(
          Invoke([](auto op) mutable { op->callback(*op, false); }));
  // ... make another request, it should fail ...
  auto fut2 = queue.async_create_rdwr_stream(
      lease.get(), &etcdserverpb::Lease::Stub::AsyncLeaseKeepAlive,
      "test/CreateLeaseKeepAlive/future/cancelled", jb::etcd::use_future());
  BOOST_REQUIRE_EQUAL(fut2.wait_for(0ms), std::future_status::ready);
  BOOST_CHECK_THROW(fut2.get(), std::exception);
}

/**
 * @test Verify creation of rdwr RPC streams is intercepted.
 */
BOOST_AUTO_TEST_CASE(mocked_grpc_interceptor_create_rdwr_stream_functor) {
  using namespace std::chrono_literals;

  // Create a null lease object, we do not need (or want) a real
  // connection for mocked operations ...
  std::shared_ptr<etcdserverpb::Lease::Stub> lease;

  using namespace jb::etcd;
  completion_queue<detail::mocked_grpc_interceptor> queue;

  // Prepare the Mock to save the asynchronous operation state,
  // normally you would simply invoke the callback in the mock action,
  // but this test wants to verify what happens if there is a delay
  // ...
  using ::testing::_;
  using ::testing::Invoke;
  EXPECT_CALL(*queue.interceptor().shared_mock, async_create_rdwr_stream(_))
      .WillRepeatedly(Invoke([](auto op) mutable { op->callback(*op, true); }));

  // ... make the request, that will post operations to the mock
  // completion queue ...
  int counter = 0;
  int* cnt = &counter;
  queue.async_create_rdwr_stream(
      lease.get(), &etcdserverpb::Lease::Stub::AsyncLeaseKeepAlive,
      "test/CreateLeaseKeepAlive/functor",
      [cnt](auto op, bool ok) { *cnt += int(ok); });

  BOOST_CHECK_EQUAL(counter, 1);
}

/**
 * @test Verify Write() operations on rdwr RPC streams are intercepted.
 */
BOOST_AUTO_TEST_CASE(mocked_grpc_interceptor_rdwr_stream_write_functor) {
  using namespace std::chrono_literals;

  // Create a null lease object, we do not need (or want) a real
  // connection for mocked operations ...
  std::shared_ptr<etcdserverpb::Lease::Stub> lease;

  using namespace jb::etcd;
  completion_queue<detail::mocked_grpc_interceptor> queue;

  // Prepare the Mock to save the asynchronous operation state,
  // normally you would simply invoke the callback in the mock action,
  // but this test wants to verify what happens if there is a delay
  // ...
  using ::testing::_;
  using ::testing::Invoke;
  EXPECT_CALL(*queue.interceptor().shared_mock, async_write(_))
      .WillRepeatedly(Invoke([](auto op) mutable { op->callback(*op, true); }));

  // ... make the request, that will post operations to the mock
  // completion queue ...
  int counter = 0;
  int* cnt = &counter;
  using stream_type = detail::async_rdwr_stream<
      etcdserverpb::LeaseKeepAliveRequest,
      etcdserverpb::LeaseKeepAliveResponse>;
  std::unique_ptr<stream_type> stream;
  etcdserverpb::LeaseKeepAliveRequest req;
  req.set_id(123456UL);
  queue.async_write(
      stream, std::move(req), "test/AsyncLeaseKeepAlive::Write/functor",
      [cnt](auto op, bool ok) { *cnt += int(ok); });

  BOOST_CHECK_EQUAL(counter, 1);
}

/**
 * @test Verify Read() operations on rdwr RPC streams are intercepted.
 */
BOOST_AUTO_TEST_CASE(mocked_grpc_interceptor_rdwr_stream_read_functor) {
  using namespace std::chrono_literals;

  // Create a null lease object, we do not need (or want) a real
  // connection for mocked operations ...
  std::shared_ptr<etcdserverpb::Lease::Stub> lease;

  using namespace jb::etcd;
  completion_queue<detail::mocked_grpc_interceptor> queue;

  // Prepare the Mock to save the asynchronous operation state,
  // normally you would simply invoke the callback in the mock action,
  // but this test wants to verify what happens if there is a delay
  // ...
  using ::testing::_;
  using ::testing::Invoke;
  EXPECT_CALL(*queue.interceptor().shared_mock, async_read(_))
      .WillRepeatedly(Invoke([](auto op) mutable { op->callback(*op, true); }));

  // ... make the request, that will post operations to the mock
  // completion queue ...
  int counter = 0;
  int* cnt = &counter;
  using stream_type = detail::async_rdwr_stream<
      etcdserverpb::LeaseKeepAliveRequest,
      etcdserverpb::LeaseKeepAliveResponse>;
  std::unique_ptr<stream_type> stream;
  queue.async_read(
      stream, "test/AsyncLeaseKeepAlive::Read/functor",
      [cnt](auto op, bool ok) { *cnt += int(ok); });

  BOOST_CHECK_EQUAL(counter, 1);
}

/**
 * @test Verify WritesDone() operations on rdwr RPC streams are intercepted.
 */
BOOST_AUTO_TEST_CASE(mocked_grpc_interceptor_rdwr_stream_writes_done_functor) {
  using namespace std::chrono_literals;

  // Create a null lease object, we do not need (or want) a real
  // connection for mocked operations ...
  std::shared_ptr<etcdserverpb::Lease::Stub> lease;

  using namespace jb::etcd;
  completion_queue<detail::mocked_grpc_interceptor> queue;

  // Prepare the Mock to save the asynchronous operation state,
  // normally you would simply invoke the callback in the mock action,
  // but this test wants to verify what happens if there is a delay
  // ...
  using ::testing::_;
  using ::testing::Invoke;
  EXPECT_CALL(*queue.interceptor().shared_mock, async_writes_done(_))
      .WillRepeatedly(Invoke([](auto op) mutable { op->callback(*op, true); }));

  // ... make the request, that will post operations to the mock
  // completion queue ...
  int counter = 0;
  int* cnt = &counter;
  using stream_type = detail::async_rdwr_stream<
      etcdserverpb::LeaseKeepAliveRequest,
      etcdserverpb::LeaseKeepAliveResponse>;
  std::unique_ptr<stream_type> stream;
  queue.async_writes_done(
      stream, "test/AsyncLeaseKeepAlive::WriteDone/functor",
      [cnt](auto op, bool ok) { *cnt += int(ok); });

  BOOST_CHECK_EQUAL(counter, 1);
}

/**
 * @test Verify WritesDone() operations with use_future() work as expected.
 */
BOOST_AUTO_TEST_CASE(mocked_grpc_interceptor_rdwr_stream_writes_done_future) {
  using namespace std::chrono_literals;

  // Create a null lease object, we do not need (or want) a real
  // connection for mocked operations ...
  std::shared_ptr<etcdserverpb::Lease::Stub> lease;

  using namespace jb::etcd;
  completion_queue<detail::mocked_grpc_interceptor> queue;

  // Prepare the Mock to save the asynchronous operation state,
  // normally you would simply invoke the callback in the mock action,
  // but this test wants to verify what happens if there is a delay
  // ...
  using ::testing::_;
  using ::testing::Invoke;
  EXPECT_CALL(*queue.interceptor().shared_mock, async_writes_done(_))
      .WillRepeatedly(Invoke([](auto op) mutable { op->callback(*op, true); }));

  // ... make the request, that will post operations to the mock
  // completion queue ...
  using stream_type = detail::async_rdwr_stream<
      etcdserverpb::LeaseKeepAliveRequest,
      etcdserverpb::LeaseKeepAliveResponse>;
  std::unique_ptr<stream_type> stream;
  auto fut = queue.async_writes_done(
      stream, "test/AsyncLeaseKeepAlive::WritesDone/future",
      jb::etcd::use_future());
  auto wait_response = fut.wait_for(0ms);
  BOOST_REQUIRE_EQUAL(wait_response, std::future_status::ready);
  BOOST_CHECK_NO_THROW(fut.get());

  // ... also test cancelations ...
  EXPECT_CALL(*queue.interceptor().shared_mock, async_writes_done(_))
      .WillRepeatedly(
          Invoke([](auto op) mutable { op->callback(*op, false); }));
  auto fut2 = queue.async_writes_done(
      stream, "test/AsyncLeaseKeepAlive::WritesDone/future/canceled",
      jb::etcd::use_future());
  wait_response = fut2.wait_for(0ms);
  BOOST_REQUIRE_EQUAL(wait_response, std::future_status::ready);
  BOOST_CHECK_THROW(fut2.get(), std::exception);
}
/**
 * @test Verify Finish() operations on rdwr RPC streams are intercepted.
 */
BOOST_AUTO_TEST_CASE(mocked_grpc_interceptor_rdwr_stream_finish_functor) {
  using namespace std::chrono_literals;

  // Create a null lease object, we do not need (or want) a real
  // connection for mocked operations ...
  std::shared_ptr<etcdserverpb::Lease::Stub> lease;

  using namespace jb::etcd;
  completion_queue<detail::mocked_grpc_interceptor> queue;

  // Prepare the Mock to save the asynchronous operation state,
  // normally you would simply invoke the callback in the mock action,
  // but this test wants to verify what happens if there is a delay
  // ...
  using ::testing::_;
  using ::testing::Invoke;
  EXPECT_CALL(*queue.interceptor().shared_mock, async_finish(_))
      .WillRepeatedly(Invoke([](auto op) mutable { op->callback(*op, true); }));

  // ... make the request, that will post operations to the mock
  // completion queue ...
  int counter = 0;
  int* cnt = &counter;
  using stream_type = detail::async_rdwr_stream<
      etcdserverpb::LeaseKeepAliveRequest,
      etcdserverpb::LeaseKeepAliveResponse>;
  std::unique_ptr<stream_type> stream;
  queue.async_finish(
      stream, "test/AsyncLeaseKeepAlive::Finish/functor",
      [cnt](auto op, bool ok) { *cnt += int(ok); });

  BOOST_CHECK_EQUAL(counter, 1);
}

/**
 * @test Verify Finish() operations with use_future() work as expected.
 */
BOOST_AUTO_TEST_CASE(mocked_grpc_interceptor_rdwr_stream_finish_future) {
  using namespace std::chrono_literals;

  // Create a null lease object, we do not need (or want) a real
  // connection for mocked operations ...
  std::shared_ptr<etcdserverpb::Lease::Stub> lease;

  using namespace jb::etcd;
  completion_queue<detail::mocked_grpc_interceptor> queue;

  // Prepare the Mock to save the asynchronous operation state,
  // normally you would simply invoke the callback in the mock action,
  // but this test wants to verify what happens if there is a delay
  // ...
  using ::testing::_;
  using ::testing::Invoke;
  EXPECT_CALL(*queue.interceptor().shared_mock, async_finish(_))
      .WillRepeatedly(Invoke([](auto op) mutable { op->callback(*op, true); }));

  // ... make the request, that will post operations to the mock
  // completion queue ...
  using stream_type = detail::async_rdwr_stream<
      etcdserverpb::LeaseKeepAliveRequest,
      etcdserverpb::LeaseKeepAliveResponse>;
  std::unique_ptr<stream_type> stream;
  auto fut = queue.async_finish(
      stream, "test/AsyncLeaseKeepAlive::Finish/future",
      jb::etcd::use_future());
  auto wait_response = fut.wait_for(0ms);
  BOOST_REQUIRE_EQUAL(wait_response, std::future_status::ready);
  BOOST_CHECK_NO_THROW(fut.get());

  // ... also test cancelations ...
  EXPECT_CALL(*queue.interceptor().shared_mock, async_finish(_))
      .WillRepeatedly(
          Invoke([](auto op) mutable { op->callback(*op, false); }));
  auto fut2 = queue.async_finish(
      stream, "test/AsyncLeaseKeepAlive::Finish/future/canceled",
      jb::etcd::use_future());
  wait_response = fut2.wait_for(0ms);
  BOOST_REQUIRE_EQUAL(wait_response, std::future_status::ready);
  BOOST_CHECK_THROW(fut2.get(), std::exception);
}
