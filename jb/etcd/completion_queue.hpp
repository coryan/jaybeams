#ifndef jb_etcd_completion_queue_hpp
#define jb_etcd_completion_queue_hpp

#include <grpc++/alarm.h>
#include <grpc++/grpc++.h>
#include <memory>

namespace jb {
namespace etcd {

/**
 * A wrapper for asynchronous unary operations
 *
 * Consider a typical gRPC:
 *
 * service Echo {
 *    rpc Echo(Request) returns (Response) {}
 * }
 *
 * When making an asynchronous request use:
 *
 * @code
 * completion_queue queue = ...;
 * auto op = queue.make_async_op<Response>([](auto op) { .. stuff .. });
 * op.rpc = stub->Echo(&op->context, req, &queue);
 * op.rpc->Finish(&op->response, &op->status, op->tag());
 * @endcode
 *
 * The jb::etcd::completion_queue will call the lambda expression
 * with the results of the operation.
 *
 * @tparam R the type of the response in the RPC operation.
 */
template <typename R>
struct async_op {
  using async_reader = grpc::ClientAsyncResponseReader<R>;

  /// Return the correct tag to use in the completion queue
  void* tag() {
    return static_cast<void*>(&callback);
  }

  grpc::ClientContext context;
  grpc::Status status;
  std::function<void()> callback;
  R response;
  std::unique_ptr<async_reader> rpc;
};

/// Create an asynchronous operation for the given functor
template <typename R, typename Functor>
std::shared_ptr<async_op<R>> make_async_op(Functor&& functor) {
  auto op = std::make_shared<async_op<R>>();
  op->callback = std::move([op, functor]() { functor(std::move(op)); });
  return op;
}

/**
 * A wrapper for asynchronous write operations on streaming RPCs.
 *
 * @tparam W the type of the request in the RPC stream.
 */
template <typename W>
struct async_write_op {
  /// Return the correct tag to use in the completion queue
  void* tag() {
    return static_cast<void*>(&callback);
  }

  grpc::ClientContext context;
  grpc::Status status;
  std::function<void()> callback;
  W request;
};

/// Create an asynchronous write operation for the given functor
template <typename W, typename Functor>
std::shared_ptr<async_write_op<W>> make_write_op(Functor&& functor) {
  auto op = std::make_shared<async_write_op<W>>();
  op->callback = std::move([op, functor]() { functor(std::move(op)); });
  return op;
}

/**
 * A wrapper for asynchronous read operations on streaming RPCs.
 *
 * @tparam R the type of the response in the RPC stream.
 */
template <typename R>
struct async_read_op {
  /// Return the correct tag to use in the completion queue
  void* tag() {
    return static_cast<void*>(&callback);
  }

  grpc::ClientContext context;
  grpc::Status status;
  std::function<void()> callback;
  R response;
};

/// Create an asynchronous read operation for the given functor.
template <typename R, typename Functor>
std::shared_ptr<async_read_op<R>> make_read_op(Functor&& functor) {
  auto op = std::make_shared<async_read_op<R>>();
  op->callback = std::move([op, functor]() { functor(std::move(op)); });
  return op;
}

/**
 * A wrapper for asynchronous unary operations
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

  grpc::ClientContext context;
  grpc::Status status;
  std::function<void()> ready_callback;
  using stream = grpc::ClientAsyncReaderWriter<W, R>;
  std::unique_ptr<stream> rpc;

  using write_op = async_write_op<W>;
  using read_op = async_read_op<R>;
};

/// Create a read-write stream
template <typename W, typename R, typename Functor>
std::shared_ptr<async_rdwr_stream<W, R>>
make_async_rdwr_stream(Functor&& functor) {
  auto stream = std::make_shared<async_rdwr_stream<W, R>>();
  stream->ready_callback = std::move([stream, functor]() { functor(stream); });
}

/**
 * A wrapper for deadline timers.
 */
struct deadline_timer {
  /// Return the correct tag to use in the completion queue
  void* tag() {
    return static_cast<void*>(&callback);
  }

  std::unique_ptr<grpc::Alarm> alarm;
  std::function<void()> callback;
};

/**
 * Wrap a gRPC completion queue.
 *
 * Alternative name: less_awful_completion_queue.  The
 * grpc::CompletionQueue is not much of an abstraction, nor is it
 * idiomatic in C++11 (and beyond).  This wrapper makes it easier to
 * write asynchronous operations that call functors (lambdas,
 * std::function<>, etc) when the operation completes.
 */
class completion_queue {
public:
  completion_queue();

  /// The underlying completion queue
  grpc::CompletionQueue& raw() {
    return queue_;
  }

  /// Run the completion queue loop.
  void run();

  /// Shutdown the completion queue loop.
  void shutdown();

  /// Call the functor when the deadline timer expires
  template <typename Functor>
  std::shared_ptr<deadline_timer> make_deadline_timer(
      std::chrono::system_clock::time_point deadline, Functor&& functor) {
    auto op = std::make_shared<deadline_timer>();
    op->callback = std::move([op, functor]() { functor(std::move(op)); });
    op->alarm = std::make_unique<grpc::Alarm>(&queue_, deadline, op->tag());
    return op;
  }

  /// Call the functor N units of time from now.
  template <typename duration_type, typename Functor>
  std::shared_ptr<deadline_timer>
  make_relative_timer(duration_type duration, Functor&& functor) {
    auto deadline = std::chrono::system_clock::now() + duration;
    return make_deadline_timer(deadline, std::move(functor));
  }

  operator grpc::CompletionQueue*() {
    return &queue_;
  }

private:
  grpc::CompletionQueue queue_;
};

} // namespace etcd
} // namespace jb

#endif // jb_etcd_completion_queue_hpp
