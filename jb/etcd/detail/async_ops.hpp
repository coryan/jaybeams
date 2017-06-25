#ifndef jb_etcd_detail_async_ops_hpp
#define jb_etcd_detail_async_ops_hpp

#include <grpc++/alarm.h>
#include <grpc++/grpc++.h>

#include <memory>

namespace jb {
namespace etcd {

class completion_queue;

namespace detail {

/**
 * Base class for all asynchronous operations.
 */
struct base_async_op {
  base_async_op() {}

  /// Make sure full destructor of derived class is called.
  virtual ~base_async_op() {}

  /**
   * Callback for the completion queue.
   *
   * It seems more natural to use a virtual function, but the derived
   * classes will just create a std::function<> to wrap the
   * user-supplied functor, so this is actually less code and more
   * efficient.
   */
  std::function<void(base_async_op&, bool)> callback;

  /// For debugging
  // TODO() - consider using a tag like a char const*
  std::string name;
};

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
struct deadline_timer : public base_async_op {
  // Safely cancel the timer, in the thread that cancels the timer we
  // simply flag it as canceled.  We only change the state in the
  // thread where the timer is fired, i.e., the thred running the
  // completion queue loop.
  void cancel() {
    alarm_->Cancel();
  }

  std::chrono::system_clock::time_point deadline;

private:
  friend class ::jb::etcd::completion_queue;
  std::unique_ptr<grpc::Alarm> alarm_;
};
  

} // namespace detail
} // namespace etcd
} // namespace jb

#endif // jb_etcd_detail_async_ops_hpp
