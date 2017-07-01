#include <jb/etcd/completion_queue.hpp>

#include <etcd/etcdserver/etcdserverpb/rpc.grpc.pb.h>

#include <jb/gmock/init.hpp>
#include <boost/test/unit_test.hpp>
#include <gmock/gmock.h>
#include <atomic>
#include <thread>

namespace std {
std::ostream& operator<<(std::ostream& os, std::future_status x) {
  if (x == std::future_status::ready) {
    return os << "[ready]";
  } else if (x == std::future_status::timeout) {
    return os << "[timeout]";
  } else if (x == std::future_status::deferred) {
    return os << "[deferred]";
  }
  return os << "[--invalid--]";
}
} // namespace std

namespace jb {
namespace etcd {

namespace detail {

struct mock_grpc_interceptor {
  mock_grpc_interceptor()
    : shared_mock(new mocked) {
  }
  template <typename C, typename M, typename op_type>
  void async_rpc(
      C* async_client, M C::*call, std::shared_ptr<op_type>& op,
      grpc::CompletionQueue* cq, void* tag) {
    shared_mock->async_rpc(op);
  }

  struct mocked {
    MOCK_CONST_METHOD1(async_rpc, void(std::shared_ptr<base_async_op> op));
  };

  std::shared_ptr<mocked> shared_mock;
};

} // namespace detail
} // namespace etcd
} // namespace jb

/**
 * @test Make sure we can mock async_rpc() calls a completion_queue.
 */
BOOST_AUTO_TEST_CASE(completion_queue_mocked_rpc) {
  using namespace std::chrono_literals;

  // Create a null lease object, we do not need (or want) a real
  // connection for mocked operations ...
  std::shared_ptr<etcdserverpb::Lease::Stub> lease;

  using namespace jb::etcd;
  completion_queue<detail::mock_grpc_interceptor> queue;

  // Prepare the Mock to save the asynchronous operation state,
  // normally you would simply invoke the callback in the mock action,
  // but this test wants to verify what happens if there is a delay
  // ...
  using ::testing::_;
  using ::testing::Invoke;
  std::shared_ptr<jb::etcd::detail::base_async_op> last_op;
  EXPECT_CALL(*queue.interceptor().shared_mock, async_rpc(_))
    .WillRepeatedly(Invoke([&last_op](auto const& op) mutable {
          last_op = op;
        }));

  // ... make the request, that will post operations to the mock
  // completion queue ...
  etcdserverpb::LeaseGrantRequest req;
  req.set_ttl(5); // in seconds
  req.set_id(0);  // let the server pick the lease_id
  auto fut = queue.async_rpc(
      lease.get(), &etcdserverpb::Lease::Stub::AsyncLeaseGrant, std::move(req),
      "test/Lease", jb::etcd::use_future());

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
BOOST_AUTO_TEST_CASE(completion_queue_mocked_rpc_cancelled) {
  using namespace std::chrono_literals;

  // Create a null lease object, we do not need (or want) a real
  // connection for mocked operations ...
  std::shared_ptr<etcdserverpb::Lease::Stub> lease;

  using namespace jb::etcd;
  completion_queue<detail::mock_grpc_interceptor> queue;

  // Prepare the Mock to save the asynchronous operation state,
  // normally you would simply invoke the callback in the mock action,
  // but this test wants to verify what happens if there is a delay
  // ...
  using ::testing::_;
  using ::testing::Invoke;
  std::shared_ptr<jb::etcd::detail::base_async_op> last_op;
  EXPECT_CALL(*queue.interceptor().shared_mock, async_rpc(_))
    .WillRepeatedly(Invoke([](auto bop) mutable {
          bop->callback(*bop, false);
        }));

  // ... make the request, that will post operations to the mock
  // completion queue ...
  etcdserverpb::LeaseGrantRequest req;
  req.set_ttl(5); // in seconds
  req.set_id(0);  // let the server pick the lease_id
  auto fut = queue.async_rpc(
      lease.get(), &etcdserverpb::Lease::Stub::AsyncLeaseGrant,
      std::move(req), "test/Lease", jb::etcd::use_future());

  // ... check that the operation was immediately cancelled ...
  BOOST_REQUIRE_EQUAL(fut.wait_for(0ms), std::future_status::ready);

  // ... and the promise was satisfied with an exception ...
  BOOST_CHECK_THROW(fut.get(), std::exception);
}
