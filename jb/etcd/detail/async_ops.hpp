#ifndef jb_etcd_detail_async_ops_hpp
#define jb_etcd_detail_async_ops_hpp

#include <grpc++/alarm.h>
#include <grpc++/grpc++.h>

#include <memory>

namespace jb {
namespace etcd {

template <typename interceptor_t>
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

/// Determine the Request and Response parameter for an RPC based on
/// the Stub signature  - mismatch case.
template <typename M>
struct async_op_requirements {
  using matches = std::false_type;
};

/// Determine the Request and Response parameter for an RPC based on
/// the Stub signature  - mismatch case.
template <typename W, typename R>
struct async_op_requirements<
    std::unique_ptr<grpc::ClientAsyncResponseReader<R>>(
        grpc::ClientContext*, W const&, grpc::CompletionQueue*)> {
  using matches = std::true_type;

  using request_type = W;
  using response_type = R;
};

/**
 * A wrapper for asynchronous unary operations.
 *
 * Please see jb::etcd::completion_queue::async_rpc for details.
 *
 * @tparam R the type of the response in the RPC operation.
 */
template <typename W, typename R>
struct async_op : public base_async_op {
  grpc::ClientContext context;
  grpc::Status status;
  W request;
  R response;
  std::unique_ptr<grpc::ClientAsyncResponseReader<R>> rpc;
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
 * A wrapper to run an asynchronous Write() operation.
 *
 * Please see the documentation
 * jb::etcd::completion_queue::async_write for details.
 */
template <typename W>
struct write_op : public base_async_op {
  W request;
};

/**
 * A wrapper to run an asynchronous Read() operation.
 *
 * Please see the documentation
 * jb::etcd::completion_queue::async_read for details.
 */
template <typename R>
struct read_op : public base_async_op {
  R response;
};

/**
 * A wrapper around read-write RPC streams.
 */
template <typename W, typename R>
struct async_rdwr_stream {
  grpc::ClientContext context;
  std::unique_ptr<grpc::ClientAsyncReaderWriter<W, R>> client;

  using write_op = ::jb::etcd::detail::write_op<W>;
  using read_op = ::jb::etcd::detail::read_op<R>;
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
      : stream(new async_rdwr_stream<W, R>) {
  }
  std::unique_ptr<async_rdwr_stream<W, R>> stream;

  using write_op = ::jb::etcd::detail::write_op<W>;
  using read_op = ::jb::etcd::detail::read_op<R>;
};

/**
 * A wrapper to run an asynchronous WritesDone() operation.
 *
 * Please see the documentation
 * jb::etcd::completion_queue::async_writes_done for details.
 */
struct writes_done_op : public base_async_op {};

/**
 * A wrapper to run an asynchronous Finish() operation.
 *
 * Please see the documentation
 * jb::etcd::completion_queue::async_finish for details.
 */
struct finish_op : public base_async_op {
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
  template<typename T>
  friend class ::jb::etcd::completion_queue;
  std::unique_ptr<grpc::Alarm> alarm_;
};

} // namespace detail
} // namespace etcd
} // namespace jb

#endif // jb_etcd_detail_async_ops_hpp
