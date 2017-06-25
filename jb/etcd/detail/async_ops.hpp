#ifndef jb_etcd_detail_async_ops_hpp
#define jb_etcd_detail_async_ops_hpp

#include <grpc++/alarm.h>
#include <grpc++/grpc++.h>

namespace jb {
namespace etcd {
namespace detail {

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
  std::function<void(bool)> callback;
  R response;
  std::unique_ptr<async_reader> rpc;
};


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

  std::function<void(bool)> callback;
  W request;
};


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

  std::function<void(bool)> callback;
  R response;
};

/**
 * A wrapper to run an asynchronous WritesDone() operation.
 *
 * To close a bi-directional streaming RPC the application usually
 * calls WritesDone() first.  This makes it easier to write such
 * asynchronous operations.
 */
struct writes_done_op {
  /// Return the correct tag to use in the completion queue
  void* tag() {
    return static_cast<void*>(&callback);
  }

  std::function<void(bool)> callback;
};


/**
 * A wrapper to run an asynchronous Finish() operation.
 *
 * When the WritesDone() operation completes an application can close
 * the a bi-direction streaming RPC using the Finish() operation.
 * This makes it easier to write such asynchronous operations.
 */
struct finish_op {
  /// Return the correct tag to use in the completion queue
  void* tag() {
    return static_cast<void*>(&callback);
  }

  grpc::Status status;
  std::function<void(bool)> callback;
};


/**
 * A wrapper for deadline timers.
 */
struct deadline_timer {
  deadline_timer(deadline_timer const&) = delete;
  deadline_timer& operator=(deadline_timer const&) = delete;

  template <typename Functor>
  static std::shared_ptr<deadline_timer> make(
      grpc::CompletionQueue* queue,
      std::chrono::system_clock::time_point deadline, Functor&& f) {
    std::shared_ptr<deadline_timer> op(new deadline_timer);
    op->callback_ = std::move([ op, functor = std::move(f) ](bool ok) {
      auto top = std::move(op);
      std::lock_guard<std::mutex> lock(op->mu_);
      auto callback = std::move(top->callback_);
      if (top->canceled_) {
        return;
      }
      top->canceled_ = true;
      functor(top);
    });
    op->alarm_ = std::make_unique<grpc::Alarm>(
        queue, deadline, static_cast<void*>(&op->callback_));
    return op;
  }

  // Safely cancel the timer, in the thread that cancels the timer we
  // simply flag it as canceled.  We only change the state in the
  // thread where the timer is fired, i.e., the thred running the
  // completion queue loop.
  void cancel() {
    std::lock_guard<std::mutex> lock(mu_);
    if (canceled_) {
      return;
    }
    alarm_->Cancel();
    canceled_ = true;
  }

private:
  deadline_timer()
      : mu_()
      , canceled_(false) {
  }

private:
  std::mutex mu_;
  bool canceled_;
  std::function<void(bool)> callback_;
  std::unique_ptr<grpc::Alarm> alarm_;
};
  

} // namespace detail
} // namespace etcd
} // namespace jb

#endif // jb_etcd_detail_async_ops_hpp
