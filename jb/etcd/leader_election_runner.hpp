#ifndef jb_etcd_leader_election_runner_hpp
#define jb_etcd_leader_election_runner_hpp

#include <jb/etcd/detail/async_ops.hpp>
#include <etcd/etcdserver/etcdserverpb/rpc.grpc.pb.h>

#include <condition_variable>
#include <mutex>

namespace jb {
namespace etcd {

/**
 * Participate in a leader election protocol.
 */
class leader_election_runner {
protected:
  leader_election_runner(
      std::uint64_t lease_id, std::unique_ptr<etcdserverpb::KV::Stub> kv_client,
      std::unique_ptr<etcdserverpb::Watch::Stub> watch_client,
      std::string const& election_name, std::string const& participant_value);

public:
  //@{
  using watcher_stream_type = detail::async_rdwr_stream<
      etcdserverpb::WatchRequest, etcdserverpb::WatchResponse>;
  using watch_write_op = watcher_stream_type::write_op;
  using watch_read_op = watcher_stream_type::read_op;
  //@}

  /**
   * The implicit state machine in a leader election participant.
   *
   * Most of the states are there for debugging, the state machine is
   * implicit after all.  Only shuttingdown and shutdown are used to
   * stop new async operations from starting.
   */
  enum class state {
    /// The initial state.
    constructing,
    /// Setting up the bi-direction stream for watchers.
    connecting,
    /// Make the initial test-and-set request to create a node with the
    /// runner's key.
    testandset,
    /// Update the value on the node if the node already existed when
    /// the runner's key.
    republish,
    /// The value on the node is updated.
    published,
    /// Getting the previous node in the election, if any.
    querying,
    /// The runner is waiting to become the leader.
    campaigning,
    /// The runner has become the leader.
    elected,
    /// The resign() operation has been called, all remote resources are
    /// released.
    resigning,
    resigned,
    /// The shutdown() member function has been called, the runner is
    /// being deleted, all local resources are released.
    shuttingdown,
    shutdown,
  };

  virtual ~leader_election_runner() noexcept(false);

  /// Return the etcd key associated with this participant
  std::string const& key() const {
    return participant_key_;
  }

  /// Return the etcd eky associated with this participant
  std::string const& value() const {
    return participant_value_;
  }

  /// Return the fetched participant revision, mostly for debugging
  std::uint64_t participant_revision() const {
    return participant_revision_;
  }

  /// Return the lease corresponding to this participant's session.
  std::uint64_t lease_id() const {
    return lease_id_;
  }

  /// Release all global resources associated with this runner
  virtual void resign() = 0;

  /// Publish a new value
  virtual void proclaim(std::string const& value) = 0;

protected:
  /// Return a nice header for all log and error messages.
  std::string log_header(char const* loc) const;

  //@{
  /**
   * @name Help derived classes track pending async ops and state changes.
   */
  /// Block until all asyncrhonous operations complete
  void async_ops_block();

  /// Return false if starting new operations is not allowed in this
  /// state (e.g. during shutdown)
  bool async_op_start(char const* msg);

  /// Call during shutdown
  bool async_op_start_shutdown(char const* msg);

  /// Indicate that an asyncrhonous op is completed
  void async_op_done(char const* msg);

  /// Return false if the state transition is invalid
  bool set_state(char const* msg, state new_state);
  //@}

protected:
  mutable std::mutex mu_;
  std::condition_variable cv_;
  state state_;
  int pending_async_ops_;

  std::string election_name_;
  std::string participant_value_;
  std::string election_prefix_;
  std::string participant_key_;
  std::uint64_t participant_revision_;
  std::uint64_t lease_id_;

  std::unique_ptr<etcdserverpb::KV::Stub> kv_client_;
  std::unique_ptr<etcdserverpb::Watch::Stub> watch_client_;
  std::shared_ptr<watcher_stream_type> watcher_stream_;
};

char const* to_str(leader_election_runner::state x);
std::ostream& operator<<(std::ostream& os, leader_election_runner::state x);

} // namespace etcd
} // namespace jb

#endif // jb_etcd_leader_election_runner_hpp
