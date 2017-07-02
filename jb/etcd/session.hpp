#ifndef jb_etcd_session_hpp
#define jb_etcd_session_hpp

#include <jb/etcd/active_completion_queue.hpp>
#include <etcd/etcdserver/etcdserverpb/rpc.grpc.pb.h>

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
      std::shared_ptr<active_completion_queue> queue,
      std::shared_ptr<grpc::Channel> etcd_channel, duration_type desired_TTL)
      : session(true, queue, etcd_channel, to_milliseconds(desired_TTL), 0) {
  }

  /**
   * Contructor with a previous lease.
   *
   * This could be useful for an application that saves its lease, shuts
   * down, and quickly restarts.  I think it is a stretch, but so easy
   * to implement that why not?
   */
  template <typename duration_type>
  session(
      std::shared_ptr<active_completion_queue> queue,
      std::shared_ptr<grpc::Channel> etcd_channel, duration_type desired_TTL,
      std::uint64_t lease_id)
      : session(
            true, queue, etcd_channel, to_milliseconds(desired_TTL), lease_id) {
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

  /**
   * Requests the least to be revoked.
   *
   * If successful, cancels all pending keep alive operations.
   */
  void revoke();

private:
  /**
   * Constructor
   *
   * TODO() - we need to add some dependency injection for unit
   * testing ...
   */
  session(
      bool, std::shared_ptr<active_completion_queue> queue,
      std::shared_ptr<grpc::Channel> etcd_channel,
      std::chrono::milliseconds desired_TTL, std::uint64_t lease_id);

  /// Requests (or renews) the lease and setup the watcher stream.
  void preamble();

  /// Shutdown the local resources.
  void shutdown();

  /// Set a timer to start the next Write/Read cycle.
  void set_timer();

  /// Handle the timer expiration, Write() a new LeaseKeepAlive request.
  void on_timeout(detail::deadline_timer const& op, bool ok);

  using ka_stream_type = detail::async_rdwr_stream<
      etcdserverpb::LeaseKeepAliveRequest,
      etcdserverpb::LeaseKeepAliveResponse>;

  /// Handle the Write() completion, schedule a new LeaseKeepAlive Read().
  void on_write(ka_stream_type::write_op& op, bool ok);

  /// Handle the Read() completion, schedule a new Timer().
  void on_read(ka_stream_type::read_op& op, bool ok);

  /// Convert the constructor argument to milliseconds.
  template <typename duration_type>
  static std::chrono::milliseconds to_milliseconds(duration_type d) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(d);
  }

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

  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<etcdserverpb::Lease::Stub> lease_client_;
  std::shared_ptr<ka_stream_type> ka_stream_;
  std::shared_ptr<active_completion_queue> queue_;

  /// The lease is assigned by etcd during the constructor
  std::uint64_t lease_id_;

  /// The requested TTL value.
  // TODO() - decide if storing the TTL in milliseconds makes any
  // sense, after all etcd uses seconds ...
  std::chrono::milliseconds desired_TTL_;

  /// etcd may tell us to use a longer (or shorter?) TTL.
  std::chrono::milliseconds actual_TTL_;

  /// The current timer, can be null when waiting for a KeepAlive response.
  std::shared_ptr<detail::deadline_timer> current_timer_;

  std::promise<bool> shutdown_completed_;
};

} // namespace etcd
} // namespace jb

#endif // jb_etcd_session_hpp
