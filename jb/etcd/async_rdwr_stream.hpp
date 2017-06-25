#ifndef jb_etcd_async_rdwr_stream_hpp
#define jb_etcd_async_rdwr_stream_hpp

#include <jb/etcd/detail/async_ops.hpp>

namespace jb {
namespace etcd {
/**
 * A wrapper for a bi-directional streaming RPC client.
 *
 * Alternative name: a less awful grpc::ClientAsyncReaderWriter<W,R>.
 *
 * Consider a typical bi-directional streaming gRPC:
 *
 * service Echo {
 *    rpc Echo(stream Request) returns (stream Response) {}
 * }
 *
 * When asynchronously creating a the stream use:
 *
 * @code
 * completion_queue queue = ...;
 * auto op = queue.make_async_rdwr_stream<Request,Response>(
 *     [this](auto op) { this->stream_ready(op); });
 * @endcode
 *
 * The jb::etcd::completion_queue will call the lambda expression
 * with the created operation when the stream is ready.
 *
 * @tparam W the type of the Requests in the streaming RPC.
 * @tparam R the type of the Responses in the streaming RPC.
 */
template <typename W, typename R>
struct async_rdwr_stream {
  /// Return the correct tag to use in the completion queue
  void* tag() {
    return static_cast<void*>(&ready_callback);
  }

  std::function<void(bool)> ready_callback;

  using client_type = grpc::ClientAsyncReaderWriter<W, R>;
  using write_op = detail::async_write_op<W>;
  using read_op = detail::async_read_op<R>;
  using write_type = W;
  using read_type = R;
};

} // namespace etcd
} // namespace jb

#endif // jb_etcd_async_rdwr_stream_hpp
