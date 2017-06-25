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
  base_async_op() {
  }

  /// Make sure full destructor of derived class is called.
  virtual ~base_async_op() {
  }

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

/// Match an operation to create ClientAsyncReaderWriter to its
/// signature - mismatch case.
template <typename M>
struct async_stream_create_requirements {
  using matches = std::false_type;
};

/// Match an operation to create ClientAsyncReaderWriter to its
/// signature - match case.
template <typename W, typename R>
struct async_stream_create_requirements<
    std::unique_ptr<grpc::ClientAsyncReaderWriter<W, R>>(
        grpc::ClientContext*, grpc::CompletionQueue*, void*)> {
  using matches = std::true_type;

  using write_type = W;
  using read_type = R;
};

/**
 * A wrapper around read-write RPC streams.
 */
template <typename W, typename R>
struct new_async_rdwr_stream {
  grpc::ClientContext context;
  std::unique_ptr<grpc::ClientAsyncReaderWriter<W, R>> client;
};

/**
 * A wrapper for a bi-directional streaming RPC client.
 *
 * Alternative name: a less awful grpc::ClientAsyncReaderWriter<W,R>.
 *
 * Please see the documentation of
 * jb::etcd::completion_queue::async_create_rdwr_stream for details.
 *
 * @tparam W the type of the requests in the streaming RPC.
 * @tparam R the type of the responses in the streaming RPC.
 *
 */
template <typename W, typename R>
struct create_async_rdwr_stream : public base_async_op {
  create_async_rdwr_stream()
      : stream(new new_async_rdwr_stream<W, R>) {
  }
  std::unique_ptr<new_async_rdwr_stream<W, R>> stream;
};

/**
 * A wrapper to run an asynchronous Write() operation.
 *
 * Please see the documentation
 * jb::etcd::completion_queue::async_write for details.
 */
template <typename W>
struct new_write_op : public base_async_op {
  W request;
};

/**
 * A wrapper to run an asynchronous Read() operation.
 *
 * Please see the documentation
 * jb::etcd::completion_queue::async_read for details.
 */
template <typename R>
struct new_read_op : public base_async_op {
  R response;
};

/**
 * A wrapper to run an asynchronous WritesDone() operation.
 *
 * Please see the documentation
 * jb::etcd::completion_queue::async_writes_done for details.
 */
struct new_writes_done_op : public base_async_op {};

/**
 * A wrapper to run an asynchronous Finish() operation.
 *
 * Please see the documentation
 * jb::etcd::completion_queue::async_finish for details.
 */
struct new_finish_op : public base_async_op {
  grpc::Status status;
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
