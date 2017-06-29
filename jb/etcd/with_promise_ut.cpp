#include <jb/etcd/completion_queue.hpp>

#include <etcd/etcdserver/etcdserverpb/rpc.grpc.pb.h>
#include <future>

#include <boost/test/unit_test.hpp>
#include <atomic>
#include <thread>

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

template <typename C, typename M, typename W>
std::shared_future<typename async_op_requirements<M>::response_type> async_rpc(
    completion_queue<>* queue, C* async_client, M C::*call, W&& request,
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

} // namespace detail
} // namespace etcd
} // namespace jb

/**
 * @test Make sure jb::etcd::completion_queue handles errors gracefully.
 */
BOOST_AUTO_TEST_CASE(completion_queue_error) {
  using namespace std::chrono_literals;

  std::string const endpoint = "localhost:2379";
  auto channel =
      grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials());
  auto lease = etcdserverpb::Lease::NewStub(channel);

  jb::etcd::completion_queue<> queue;
  std::thread t([&queue]() { queue.run(); });

  using namespace jb::etcd;
  etcdserverpb::LeaseGrantRequest req;
  req.set_ttl(5); // in seconds
  req.set_id(0);  // let the server pick the lease_id
  auto fut = detail::async_rpc(
      &queue, lease.get(), &etcdserverpb::Lease::Stub::AsyncLeaseGrant,
      std::move(req), "test/Lease", jb::etcd::use_future());

  // ... block ...
  auto response = fut.get();

  queue.shutdown();
  t.join();
}
