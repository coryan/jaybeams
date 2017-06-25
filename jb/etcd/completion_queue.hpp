#ifndef jb_etcd_completion_queue_hpp
#define jb_etcd_completion_queue_hpp

#include <jb/etcd/async_rdwr_stream.hpp>

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>

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

  /// Create a new reader-writer asynchronous stream
  template<typename R, typename W, typename Functor>
  void async_create_rdwr_stream(Functor&& functor) {}

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
      std::chrono::system_clock::time_point deadline, Functor&& functor) {
    return detail::deadline_timer::make(&queue_, deadline, std::move(functor));
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

/// Create an asynchronous write operation for the given functor
template <typename W, typename Functor>
std::shared_ptr<detail::async_write_op<W>> make_write_op(Functor&& f) {
  auto op = std::make_shared<detail::async_write_op<W>>();
  op->callback = std::move([ op, functor = std::move(f) ](bool ok) {
    auto tmp = std::move(op);
    functor(tmp);
    (void)std::move(tmp->callback);
  });
  return op;
}

/// Create an asynchronous read operation for the given functor.
template <typename R, typename Functor>
std::shared_ptr<detail::async_read_op<R>> make_read_op(Functor&& f) {
  auto op = std::make_shared<detail::async_read_op<R>>();
  op->callback = std::move([ op, functor = std::move(f) ](bool ok) {
    auto callback = std::move(op->callback);
    functor(std::move(op));
  });
  return op;
}

/// Create an asyncrhonous WritesDone operation wrapper.
template <typename Functor>
std::shared_ptr<detail::writes_done_op> make_writes_done_op(Functor&& functor) {
  auto op = std::make_shared<detail::writes_done_op>();
  op->callback = std::move([op, functor](bool ok) {
      auto tmp = std::move(op);
      functor(tmp);
      (void)std::move(tmp->callback);
  });
  return op;
}

/// Create an asynchronous Finish operation wrapper.
template <typename Functor>
std::shared_ptr<detail::finish_op> make_finish_op(Functor&& functor) {
  auto op = std::make_shared<detail::finish_op>();
  op->callback = std::move([op, functor](bool ok) {
    auto callback = std::move(op->callback);
    functor(std::move(op));
  });
  return op;
}
  
/// Create a read-write stream
template <typename W, typename R, typename Functor>
std::shared_ptr<async_rdwr_stream<W, R>>
make_async_rdwr_stream(Functor&& functor) {
  auto stream = std::make_shared<async_rdwr_stream<W, R>>();
  stream->ready_callback = std::move([stream, functor](bool ok) {
    auto callback = std::move(stream->ready_callback);
    functor(stream);
  });
  return stream;
}

} // namespace etcd
} // namespace jb

#endif // jb_etcd_completion_queue_hpp
