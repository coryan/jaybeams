#ifndef jb_etcd_completion_queue_hpp
#define jb_etcd_completion_queue_hpp

#include <jb/etcd/detail/async_ops.hpp>

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace jb {
namespace etcd {

/**
 * Wrap a gRPC completion queue.
 *
 * Alternative name: less_awful_completion_queue.  The
 * grpc::CompletionQueue is not much of an abstraction, nor is it
 * idiomatic in C++11 (and its successors).  This wrapper makes it easier to
 * write asynchronous operations that call functors (lambdas,
 * std::function<>, etc) when the operation completes.
 */
class completion_queue {
public:
  /// Stop the loop periodically to check if we should shutdown.
  // TODO() - the timeout should be configurable ...
  static std::chrono::milliseconds constexpr loop_timeout{250};

  completion_queue();
  ~completion_queue();

  /// The underlying completion queue
  grpc::CompletionQueue& raw() {
    return queue_;
  }

  /// Run the completion queue loop.
  void run();

  /// Shutdown the completion queue loop.
  void shutdown();

  /**
   * Create a new asynchronous read-write stream and call the functor
   * when it is constructed and ready.
   *
   * Consider a typical bi-directional streaming gRPC:
   *
   * @code
   * service Echo {
   *    rpc Echo(stream Request) returns (stream Response) {}
   * }
   * @endcode
   *
   * When asynchronously creating the stream use:
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
  template <typename C, typename M, typename Functor>
  void async_create_rdwr_stream(
      C* async_client, M C::*call, std::string name, Functor&& f) {
    using requirements = detail::async_stream_create_requirements<M>;
    static_assert(
        requirements::matches::value,
        "The member function argument must meet its requirements."
        "  Signature should match: "
        "std::unique_ptr<grpc::ClientAsyncReaderWriter<W, R>>("
        "grpc::ClientContext*,grpc::CompletionQueue*,void*)");
    using write_type = typename requirements::write_type;
    using read_type = typename requirements::read_type;

    using op_type = detail::create_async_rdwr_stream<write_type, read_type>;
    auto op = std::make_shared<op_type>();
    op->callback = [functor = std::move(f)](
        detail::base_async_op & bop, bool ok) {
      auto& op = dynamic_cast<op_type&>(bop);
      functor(std::move(op.stream), ok);
    };
    op->name = std::move(name);
    void* tag = register_op("async_create_rdwr_stream()", op);
    op->stream->client = (async_client->*call)(&op->stream->context, cq(), tag);
  }

  /**
   * Make an asynchronous call to Write() and call the functor
   * when it is completed.
   */
  template <typename W, typename R, typename Functor>
  std::shared_ptr<detail::write_op<W>> async_write(
      std::unique_ptr<detail::async_rdwr_stream<W, R>>& stream, W&& request,
      std::string name, Functor&& f) {
    using op_type = detail::write_op<W>;
    auto op = std::make_shared<op_type>();
    op->request.Swap(&request);
    op->callback = [functor = std::move(f)](
        detail::base_async_op & bop, bool ok) {
      auto& op = dynamic_cast<op_type&>(bop);
      functor(op, ok);
    };
    op->name = std::move(name);
    void* tag = register_op("async_writes_done()", op);
    stream->client->Write(op->request, tag);
    return op;
  }

  /**
   * Make an asynchronous call to Read() and call the functor
   * when it is completed.
   */
  template <typename W, typename R, typename Functor>
  std::shared_ptr<detail::read_op<R>> async_read(
      std::unique_ptr<detail::async_rdwr_stream<W, R>>& stream,
      std::string name, Functor&& f) {
    using op_type = detail::read_op<R>;
    auto op = std::make_shared<op_type>();
    op->callback = [functor = std::move(f)](
        detail::base_async_op & bop, bool ok) {
      auto& op = dynamic_cast<op_type&>(bop);
      functor(op, ok);
    };
    op->name = std::move(name);
    void* tag = register_op("async_writes_done()", op);
    stream->client->Read(&op->response, tag);
    return op;
  }

  /**
   * Make an asynchronous call to WritesDone() and call the functor
   * when it is completed.
   */
  template <typename W, typename R, typename Functor>
  std::shared_ptr<detail::writes_done_op> async_writes_done(
      std::unique_ptr<detail::async_rdwr_stream<W, R>>& stream,
      std::string name, Functor&& f) {
    using op_type = detail::writes_done_op;
    auto op = std::make_shared<op_type>();
    op->callback = [functor = std::move(f)](
        detail::base_async_op & bop, bool ok) {
      auto& op = dynamic_cast<op_type&>(bop);
      functor(op, ok);
    };
    op->name = std::move(name);
    void* tag = register_op("async_writes_done()", op);
    stream->client->WritesDone(tag);
    return op;
  }

  /**
   * Make an asynchronous call to Finish() and call the functor
   * when it is completed.
   */
  template <typename W, typename R, typename Functor>
  std::shared_ptr<detail::finish_op> async_finish(
      std::unique_ptr<detail::async_rdwr_stream<W, R>>& stream,
      std::string name, Functor&& f) {
    using op_type = detail::finish_op;
    auto op = std::make_shared<op_type>();
    op->callback = [functor = std::move(f)](
        detail::base_async_op & bop, bool ok) {
      auto& op = dynamic_cast<op_type&>(bop);
      functor(op, ok);
    };
    op->name = std::move(name);
    void* tag = register_op("async_writes_done()", op);
    stream->client->Finish(&op->status, tag);
    return op;
  }

  /**
   * Call the functor when the deadline timer expires.
   *
   * Notice that system_clock is not guaranteed to be monotonic, which
   * makes it a poor choice in some cases.  gRPC is not dealing with
   * time intervals small enough to make a difference, so it is Okay,
   * I guess.
   */
  template <typename Functor>
  std::shared_ptr<detail::deadline_timer> make_deadline_timer(
      std::chrono::system_clock::time_point deadline, std::string name,
      Functor&& f) {
    auto op = std::make_shared<detail::deadline_timer>();
    op->callback = [functor = std::move(f)](
        detail::base_async_op & bop, bool ok) {
      detail::deadline_timer& op = dynamic_cast<detail::deadline_timer&>(bop);
      functor(op, ok);
    };
    op->deadline = deadline;
    op->name = std::move(name);
    void* tag = register_op("deadline_timer()", op);
    op->alarm_ = std::make_unique<grpc::Alarm>(cq(), deadline, tag);
    return op;
  }

  /// Call the functor N units of time from now.
  template <typename duration_type, typename Functor>
  std::shared_ptr<detail::deadline_timer>
  make_relative_timer(duration_type duration, Functor&& functor) {
    auto deadline = std::chrono::system_clock::now() + duration;
    return make_deadline_timer(deadline, std::move(functor));
  }

  operator grpc::CompletionQueue*() {
    return &queue_;
  }

private:
  /// The underlying completion queue pointer for the gRPC APIs.
  grpc::CompletionQueue* cq() {
    return &queue_;
  }

  /// Save a newly created operation and return its gRPC tag.
  void*
  register_op(char const* where, std::shared_ptr<detail::base_async_op> op);

private:
  mutable std::mutex mu_;
  using pending_ops_type =
      std::unordered_map<std::intptr_t, std::shared_ptr<detail::base_async_op>>;
  pending_ops_type pending_ops_;

  grpc::CompletionQueue queue_;
  std::atomic<bool> shutdown_;
};

/// Create an asynchronous operation for the given functor
template <typename R, typename Functor>
std::shared_ptr<detail::async_op<R>> make_async_op(Functor&& functor) {
  auto op = std::make_shared<detail::async_op<R>>();
  op->callback = std::move([op, functor](bool ok) {
    auto tmp = std::move(op);
    functor(tmp);
    (void)move(tmp->callback);
  });
  return op;
}

} // namespace etcd
} // namespace jb

#endif // jb_etcd_completion_queue_hpp
