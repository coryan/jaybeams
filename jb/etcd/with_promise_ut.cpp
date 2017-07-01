#include <jb/etcd/completion_queue.hpp>

#include <etcd/etcdserver/etcdserverpb/rpc.grpc.pb.h>

#include <jb/gmock/init.hpp>
#include <boost/test/unit_test.hpp>
#include <gmock/gmock.h>
#include <atomic>
#include <future>
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

/// A struct to indicate the APIs should return futures instead of
/// invoking a callback.
struct use_future {};

namespace detail {

/**
 * Create a new asynchronous read-write stream and return a promise
 * to wait until it is constructed and ready.
 *
 * Consider a typical bi-directional streaming gRPC:
 *
 * @code
 * service Echo {
 *    rpc Echo(stream Request) returns (stream Response) {}
 * }
 * @endcode
 *
 * If you want to use that stream asynchronously you must also use
 * the asynchronous APIs to create it.  Sometimes it is necessary to
 * block until the asynchronous creation completes.  This function
 * makes it easy to do so, taking care of the promise creation and
 * reporting.
 *
 * @code
 * completion_queue queue = ...;
 * std::unique_ptr<Echo::Stub> client = ...;
 * auto op = queue.async_create_rdwr_stream(
 *     "debug string", stub, Echo::Stub::AsyncEcho,
 *     [](auto stream, bool ok) { });
 * @endcode
 *
 * The jb::etcd::completion_queue will call the lambda expression you
 * provided.  The @a ok flag indicates if the operation was canceled.
 * The @a stream parameter will be of type:
 *
 * @code
 * std::unique_ptr<async_rdwr_stream<Request, Response>>
 * @endcode
 *
 * This function deduces the type of read-write stream to create
 * based on the member function argument.  Typically it would be
 * used as follows:
 */
template <typename C, typename M>
std::shared_future<
    std::unique_ptr<typename async_stream_create_requirements<M>::stream_type>>
async_create_rdwr_stream(
    completion_queue<>* queue, C* async_client, M C::*call, std::string name,
    use_future) {
  using ret_type = std::unique_ptr<
      typename async_stream_create_requirements<M>::stream_type>;
  auto promise = std::make_shared<std::promise<ret_type>>();
  auto async = queue->async_create_rdwr_stream(
      async_client, call, name, [promise](auto stream, bool ok) {
        if (not ok) {
          promise->set_exception(std::make_exception_ptr(
              std::runtime_error("async create_rdwr_stream cancelled")));
          return;
        }
        promise->set_value(std::move(stream));
      });
  return promise->get_future().share();
}

template <typename completion_queue_type, typename C, typename M, typename W>
std::shared_future<typename async_op_requirements<M>::response_type> async_rpc(
    completion_queue_type* queue, C* async_client, M C::*call, W&& request,
    std::string name, use_future) {
  auto promise = std::make_shared<
      std::promise<typename async_op_requirements<M>::response_type>>();
  queue->async_rpc(
      async_client, call, std::move(request), std::move(name),
      [promise](auto& op, bool ok) {
        if (not ok) {
          promise->set_exception(std::make_exception_ptr(
              std::runtime_error("async rpc cancelled")));
          return;
        }
        // TODO() - we would want to use std::move(), but (a)
        // protobufs do not have move semantics (yuck), and (b) we
        // have a const& op parameter, so we would need to change that
        // too.
        promise->set_value(op.response);
      });
  return promise->get_future().share();
}

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

  std::string const endpoint = "localhost:2379";
  auto channel =
      grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials());
  auto lease = etcdserverpb::Lease::NewStub(channel);

  using namespace jb::etcd;
  completion_queue<detail::mock_grpc_interceptor> queue;
  std::thread t([&queue]() { queue.run(); });

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
  auto fut = detail::async_rpc(
      &queue, lease.get(), &etcdserverpb::Lease::Stub::AsyncLeaseGrant,
      std::move(req), "test/Lease", jb::etcd::use_future());

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

  queue.shutdown();
  t.join();
}
