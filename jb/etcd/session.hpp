#ifndef jb_etcd_session_hpp
#define jb_etcd_session_hpp

#include <jb/etcd/active_completion_queue.hpp>
#include <jb/etcd/client_factory.hpp>
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
      std::shared_ptr<client_factory> factory, std::string const& etcd_endpoint,
      duration_type desired_TTL)
      : session(
            true, queue, factory, etcd_endpoint, to_milliseconds(desired_TTL),
            0) {
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
      std::shared_ptr<client_factory> factory, std::string const& etcd_endpoint,
      duration_type desired_TTL, std::uint64_t lease_id)
      : session(
            true, queue, factory, etcd_endpoint, to_milliseconds(desired_TTL),
            lease_id) {
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
      std::shared_ptr<client_factory> factory, std::string const& etcd_endpoint,
      std::chrono::milliseconds desired_TTL, std::uint64_t lease_id);

  /// Requests (or renews) the lease and setup the watcher stream.
  void preamble();

  /// Shutdown the local resources.
  void shutdown();

  /// Set a timer to start the next Write/Read cycle.
  void set_timer();

  /// Handle the timer expiration, Write() a new LeaseKeepAlive request.
  void on_timeout(std::shared_ptr<detail::deadline_timer> op);

  using ka_stream_type = async_rdwr_stream<
      etcdserverpb::LeaseKeepAliveRequest,
      etcdserverpb::LeaseKeepAliveResponse>;

  /// Handle the Write() completion, schedule a new LeaseKeepAlive Read().
  void on_write(std::shared_ptr<ka_stream_type::write_op> op);

  /// Handle the Read() completion, schedule a new Timer().
  void on_read(std::shared_ptr<ka_stream_type::read_op> op);

  /// Handle the WritesDone() completion, schedule a Finish()
  void on_writes_done(
      std::shared_ptr<detail::writes_done_op> writes_done,
      std::promise<bool>& done);

  /// Handle the Finish() completion.
  void
  on_finish(std::shared_ptr<detail::finish_op> op, std::promise<bool>& done);

  /// Convert the constructor argument to milliseconds.
  template <typename duration_type>
  static std::chrono::milliseconds to_milliseconds(duration_type d) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(d);
  }

  /// Convert status errors into a C++ exception
  std::runtime_error error_grpc_status(
      char const* where, grpc::Status const& status,
      google::protobuf::Message const* res = nullptr,
      google::protobuf::Message const* req = nullptr) const;

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

  std::shared_ptr<client_factory> client_;
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<etcdserverpb::Lease::Stub> lease_client_;
  grpc::ClientContext keep_alive_stream_context_;
  std::unique_ptr<ka_stream_type::client_type> keep_alive_stream_;
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
