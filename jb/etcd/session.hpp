#ifndef jb_etcd_session_hpp
#define jb_etcd_session_hpp

#include <etcd/etcdserver/etcdserverpb/rpc.grpc.pb.h>
#include <grpc++/alarm.h>
#include <grpc++/grpc++.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <mutex>

namespace jb {
namespace etcd {

class session {
public:
  /// How many KeepAlive requests we send per TTL cyle.
  // TODO() - the magic number 5  should be a configurable parameter.
  static int constexpr keep_alives_per_ttl = 5;

  /**
   * The implicit state machine in the session.
   *
   * Sessions have a ver simple state machine:
   * -# constructing: the initial state.  Transitions to @c connecting
   * -# connecting: the session has obtained a lease id and is now
   *    establishing a reader-writer stream to keep the lease alive.
   *    Transitions to connected or shutting down.
   * -# connected: the session is connected, in this state the session
   *    sends period KeepAlive requests to renew the lease.
   *    Transitions to shutting down.
   * -# shuttingdown: the ssession is being shut down.  Any pending
   *    KeepAlive requests are canceled, their responses, if any, are
   *    received but trigger no further action.  The connection
   *    half-closes the reader-writer stream.  When the reader-writer
   *    stream is closed, a LeaseRevoke request is sent.  When that
   *    request succeeds the CompletionQueue is shutdown, and the
   *    object can be destructed.  Transitions to shutdown at the end
   *    of that sequence.
   * TODO() - consider more states for shutting down.
   */
  enum class state {
    constructing,
    connecting,
    connected,
    shuttingdown,
    shutdown,
  };

  /// Constructor
  template <typename duration_type>
  session(
      std::shared_ptr<grpc::Channel> etcd_service, duration_type desired_TTL)
      : session(
            etcd_service,
            std::chrono::duration_cast<std::chrono::milliseconds>(desired_TTL),
            true) {
  }

  /**
   * Destroy a session, releasing any local resources.
   *
   * The destructor just ensures that local resources (threads, RPC
   * streams, etc.) are released.  No attempt is made to release
   * resources in etcd, such as the lease.  If the application wants
   * to release the resources it should call revoke() and join any
   * threads that have called run().
   */
  ~session() noexcept(false);

  /**
   * The session's lease.
   *
   * The lease may expire or otherwise become invalid while the
   * session is shutting down.  Applications should avoid using the
   * lease after calling initiate_shutdown().
   */
  std::uint64_t lease_id() const {
    return lease_id_;
  }

  state current_state() const {
    std::lock_guard<std::mutex> guard(mu_);
    return state_;
  }

  /// Start the shutdown process ...
  void initiate_shutdown();

  /// The main thread
  void run();

  /// Revoke the lease, implicitly initiates a shutdown if sucessful.
  void revoke();

  /// TODO() - the completion queue should be an argument to the
  /// constructor ...
  grpc::CompletionQueue& queue() {
    return queue_;
  }

private:
  /**
   * Constructor
   *
   * TODO() - we need to add some dependency injection for unit
   * testing ...
   */
  session(
      std::shared_ptr<grpc::Channel> etcd_service,
      std::chrono::milliseconds desired_TTL, bool);

  /// Set a timer to start the next Write/Read cycle.
  void set_timer();

  /// Handle the timer expiration, Write() a KeepAlive request.
  void on_timeout();

  /// Handle the Write() completion, schedule a new Read().
  void on_write();

  /// Handle the Read() completion, schedule a new Timer().
  void on_read();

  /// Handle the WritesDone() completion, schedule a Finish()
  void on_writes_done();

  /// Handle the Finish() completion, schedule a LeaseRevoke()
  void on_finish();

  typedef void (session::*callback_member)();
  std::function<void()> make_tag(callback_member member);

private:
  /// The usual mutex thing.
  mutable std::mutex mu_;

  /**
   * Implement the state machine.
   *
   * See the documentation of the @c state enum for details of the
   * transition table.
   * TODO() - consider using one of the Boost.StateMachine classes.
   */
  state state_;

  std::shared_ptr<grpc::Channel> etcd_service_;

  /// The lease is assigned by etcd during the constructor
  std::uint64_t lease_id_;

  /// The requested TTL value.
  std::chrono::milliseconds desired_TTL_;

  /// etcd may tell us to use a longer (or shorter?) TTL.
  std::chrono::milliseconds actual_TTL_;

  // TODO() - I do not think the queue belongs in this object, seems
  // like it should be a parameter, if nothing else for dependency
  // injection ...
  grpc::CompletionQueue queue_;
  std::unique_ptr<etcdserverpb::Lease::Stub> stub_;

  /// The context for the asynchronous KeepAlive request.
  grpc::ClientContext kactx_;
  using ReaderWriter = grpc::ClientAsyncReaderWriter<
      etcdserverpb::LeaseKeepAliveRequest,
      etcdserverpb::LeaseKeepAliveResponse>;
  std::unique_ptr<ReaderWriter> rdwr_;

  //@{
  /**
   * Hold data for asynchronous operations.
   */
  std::unique_ptr<grpc::Alarm> alarm_;
  etcdserverpb::LeaseKeepAliveRequest ka_req_;
  etcdserverpb::LeaseKeepAliveResponse ka_res_;
  grpc::Status finish_status_;
  std::promise<bool> shutdown_completed_;
  //@}

  //@{
  /**
   * Tags / callback for asynchronous operations.
   *
   * gRPC++ tags are awkward, they should have used realcallbacks, ala
   * Boost.ASIO.  We need something that is convertible to and from a
   * void*.  We could create an object for each asynchronous
   * operation, but that is slow, and also extra weird.  Instead we
   * create a number of member objects, one for each asynchronous
   * operation.  Each member object is a std::function<void()>, which
   * we can call from the completion queue loop.  The other
   * alternatives I though about do not work:
   *   - Pointer to member functions are not covertible to void*.
   *   - A pointer to a lambda would not work, because we need to
   *     capture at least the [this] pointer, and pointer to
   *     lambdas with captures are *not* pointer to functions.
   *   - Also the ponter to lambda would need to be stored
   *     somewhere.
   */
  std::function<void()> tag_set_timer_;
  std::function<void()> tag_on_timeout_;
  std::function<void()> tag_on_write_;
  std::function<void()> tag_on_read_;
  std::function<void()> tag_on_writes_done_;
  std::function<void()> tag_on_finish_;
  //@}
};

} // namespace etcd
} // namespace jb

#endif // jb_etcd_session_hpp
