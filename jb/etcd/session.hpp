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
  //@{
  /// @name type traits

  /// The type of the bi-directional RPC stream for keep alive messages
  using ka_stream_type = detail::async_rdwr_stream<
      etcdserverpb::LeaseKeepAliveRequest,
      etcdserverpb::LeaseKeepAliveResponse>;

  /// The preferred units for measuring time in this class
  using duration_type = std::chrono::milliseconds;
  //@}

  /// How many KeepAlive requests we send per TTL cyle.
  // TODO() - the magic number 5  should be a configurable parameter.
  static int constexpr keep_alives_per_ttl = 5;

  /**
   * The implicit state machine in the session.
   *
   * Sessions have a ver simple state machine:
   */
  enum class state {
    /**
     * The initial state.
     *
     * Transitions to @c connecting
     */
    constructing,
    /**
     * The session has obtained a lease id and is now
     * establishing a reader-writer stream to keep the lease alive.
     *
     * Transitions to @c connected or @c shuttingdown.
     */
    connecting,
    /**
     * The session is connected, in this state the session
     *    sends period KeepAlive requests to renew the lease.
     *
     *    Transitions to @c shuttingdown.
     */
    connected,

    /**
     * The ssession is being shut down.
     *
     * Any pending KeepAlive requests are canceled, their responses,
     * if any, are received but trigger no further action.  The
     * connection half-closes the reader-writer stream.  When the
     * reader-writer stream is closed, a LeaseRevoke request is sent.
     * When that request succeeds the CompletionQueue is shutdown, and
     * the object can be destructed.
     *
     * Transitions to @c shutdown at the end of that sequence.
     */
    shuttingdown,

    /**
     * Final state.
     *
     * The session cannot transition out of this state.
     */
    shutdown,
  };

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

  std::chrono::milliseconds actual_TTL() const {
    return actual_TTL_;
  }

  state current_state() const {
    std::lock_guard<std::mutex> guard(mu_);
    return state_;
  }

  /**
   * Requests the lease to be revoked.
   *
   * If successful, all pending keep alive operations have been
   * canceled and the lease is revoked on the server.
   */
  virtual void revoke() = 0;

  /// Convert a duration to the preferred units in this class
  template <typename other_duration_type>
  static duration_type convert_duration(other_duration_type d) {
    return std::chrono::duration_cast<duration_type>(d);
  }

protected:
  /// Only derived classes can construct this object.
  session(
      std::unique_ptr<etcdserverpb::Lease::Stub> lease_stub,
      std::chrono::milliseconds desired_TTL, std::uint64_t lease_id);

protected:
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

  std::unique_ptr<etcdserverpb::Lease::Stub> lease_client_;
  std::shared_ptr<ka_stream_type> ka_stream_;

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
};

std::ostream& operator<<(std::ostream& os, session::state x);

} // namespace etcd
} // namespace jb

#endif // jb_etcd_session_hpp
