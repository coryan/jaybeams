#ifndef jb_etcd_leader_election_participant_hpp
#define jb_etcd_leader_election_participant_hpp

#include <jb/etcd/active_completion_queue.hpp>
#include <jb/etcd/client_factory.hpp>
#include <jb/etcd/session.hpp>

#include <atomic>
#include <condition_variable>
#include <future>

namespace jb {
namespace etcd {

/**
 * Participate in a leader election protocol.
 */
class leader_election_participant {
public:
  /// Constructor, blocks until this participant becomes the leader.
  template <typename duration_type>
  leader_election_participant(
      std::shared_ptr<active_completion_queue> queue,
      std::shared_ptr<client_factory> client, std::string const& etcd_endpoint,
      std::string const& election_name, std::string const& participant_value,
      duration_type d, std::uint64_t lease_id = 0)
      : leader_election_participant(
            true, queue, client, etcd_endpoint, election_name,
            participant_value, std::make_shared<session>(
                                   queue, client, etcd_endpoint, d, lease_id)) {
    // ... block until this participant becomes the leader ...
    campaign();
  }

  /// Constructor, non-blocking, calls the callback when elected.
  template <typename duration_type, typename Functor>
  leader_election_participant(
      std::shared_ptr<active_completion_queue> queue,
      std::shared_ptr<client_factory> client_factory,
      std::string const& etcd_endpoint, std::string const& election_name,
      std::string const& participant_value, Functor const& elected_callback,
      duration_type d, std::uint64_t lease_id = 0)
      : leader_election_participant(
            true, queue, client_factory, etcd_endpoint, election_name,
            participant_value, std::move(elected_callback), d, lease_id) {
  }

  /// Constructor, non-blocking, calls the callback when elected.
  template <typename duration_type, typename Functor>
  leader_election_participant(
      std::shared_ptr<active_completion_queue> queue,
      std::shared_ptr<client_factory> client, std::string const& etcd_endpoint,
      std::string const& election_name, std::string const& participant_value,
      Functor&& elected_callback, duration_type d, std::uint64_t lease_id = 0)
      : leader_election_participant(
            true, queue, client, etcd_endpoint, election_name,
            participant_value, std::make_shared<session>(
                                   queue, client, etcd_endpoint, d, lease_id)) {
    campaign(elected_callback);
  }

  /**
   * Release local resources.
   *
   * The destructor makes sure the *local* resources are released,
   * including connections to the etcd server, and pending
   * operations.  It makes no attempt to resign from the election, or
   * delete the keys in etcd, or to gracefully revoke the etcd leases.
   *
   * The application should call resign() to release the resources
   * held in the etcd server *before* the destructor is called.
   */
  ~leader_election_participant() noexcept(false);

  /**
   * The implicit state machine in a leader election participant.
   *
   * Most of the states are there for debugging, the state machine is
   * implicit after all.  Only shuttingdown and shutdown are used to
   * stop new async operations from starting.
   *
   * -# constructing: the initial state.
   * -# connecting: setting up bi-direction stream for watchers.
   * -# testandtest:
   * -# republish:
   * -# published:
   * -# querying: discover the previous participant in the election.
   * -# campaigning: the previous participant is known and the
   *    watchers are setup
   * -# elected
   * -# resigning
   * -# shuttingdown
   * -# shutdown
   *
   */
  enum class state {
    constructing,
    connecting,
    testandset,
    republish,
    published,
    querying,
    campaigning,
    elected,
    resigning,
    resigned,
    shuttingdown,
    shutdown,
  };

  /// Return the etcd key associated with this participant
  std::string const& key() const {
    return participant_key_;
  }

  /// Return the etcd eky associated with this participant
  std::string const& value() const {
    return participant_value_;
  }

  /// Resign from the election, terminate the internal loops
  void resign();

  /// Change the published value
  void proclaim(std::string const& new_value);

private:
  /// Refactor common code to public constructors ...
  leader_election_participant(
      bool, std::shared_ptr<active_completion_queue> queue,
      std::shared_ptr<client_factory> client_factory,
      std::string const& etcd_endpoint, std::string const& election_name,
      std::string const& participant_value, std::shared_ptr<session> session);

  /**
   * Runs the operations before starting the election campaign.
   *
   * This function can throw exceptions which means the campaign was
   * never even started.
   */
  void preamble();

  /**
   * Gracefully shutdown a partially or fully constructed instance
   *
   * After the thread running the event loop is launched the
   * destruction process for this class is complicated.  The thread
   * must exit, or std::thread will call std::terminate because it was
   * not joined.
   * To terminate the thread we need to finish the completion queue
   * loop.
   * That requires terminating any pending operations, or the
   * completion queue calls abort().
   */
  void shutdown();

  /// Block the calling thread until the participant has become the
  /// leader.
  void campaign();

  /// Kick-off a campaign and call a functor when elected
  template <typename Functor>
  void campaign(Functor&& callback) {
    campaign_impl(std::function<void(std::future<bool>&)>(std::move(callback)));
  }

  /**
   * Refactor template code via std::function
   *
   * The main "interface" is the campaing() template member functions,
   * but we loath duplicating that much code here, so refactor with a
   * std::function<>.  The cost of such functions is higher, but
   * leader election is not a fast operation.
   */
  void campaign_impl(std::function<void(std::future<bool>&)>&& callback);

  /// Refactor code common to proclaim() and preamble()
  etcdserverpb::TxnResponse publish_value(
      std::string const& value, etcdserverpb::RequestOp const& failure_op);

  /// Refactor code to perform a Txn() request.
  etcdserverpb::TxnResponse commit(etcdserverpb::TxnRequest const& req);

  /// Called when the Range() operation in the kv_client completes.
  void on_range_request(
      detail::async_op<
          etcdserverpb::RangeRequest, etcdserverpb::RangeResponse> const& op,
      bool ok);

  using watcher_stream_type = detail::async_rdwr_stream<
      etcdserverpb::WatchRequest, etcdserverpb::WatchResponse>;
  using watch_write_op = watcher_stream_type::write_op;
  using watch_read_op = watcher_stream_type::read_op;

  /// Called when a Write() operation that creates a watcher completes.
  void on_watch_create(
      watch_write_op const& op, bool ok, std::string const& watched_key,
      std::uint64_t watched_revision);

  /// Called when a Write() operation that cancels a watcher completes.
  void
  on_watch_cancel(watch_write_op const& op, bool ok, std::uint64_t watched_id);

  /// Called when a Read() operation in the watcher stream completes.
  void on_watch_read(
      watch_read_op const& op, bool ok, std::string const& key,
      std::uint64_t revision);

  /// Called when the WritesDone() operation in the watcher stream completes.
  void on_writes_done(
      detail::writes_done_op const& op, bool ok, std::promise<bool>& done);

  /// Called when the Finish() operation in the watcher stream completes.
  void
  on_finish(detail::finish_op const& op, bool ok, std::promise<bool>& done);

  /// Check if the election has finished, if so invoke the callbacks.
  void check_election_over_maybe();

  // Invoke the callback, notice that the callback is invoked only once.
  void make_callback();

  /// Return an exception with the @a status details
  std::runtime_error error_grpc_status(
      char const* where, grpc::Status const& status,
      google::protobuf::Message const* res = nullptr,
      google::protobuf::Message const* req = nullptr) const;

  /// Return the lease corresponding to this participant's session.
  std::uint64_t lease_id() {
    return session_->lease_id();
  }

  void async_ops_block();
  bool async_op_start(char const* msg);
  bool async_op_start_shutdown(char const* msg);
  void async_op_done(char const* msg);

  bool set_state(char const* msg, state new_state);

  // Compute the end of the range for a prefix search.
  static std::string prefix_end(std::string const& prefix);

private:
  std::shared_ptr<active_completion_queue> queue_;
  std::shared_ptr<client_factory> client_;
  std::shared_ptr<grpc::Channel> channel_;
  std::shared_ptr<session> session_;
  std::unique_ptr<etcdserverpb::KV::Stub> kv_client_;
  std::unique_ptr<etcdserverpb::Watch::Stub> watch_client_;
  std::unique_ptr<watcher_stream_type> watcher_stream_;
  std::string election_name_;
  std::string participant_value_;
  std::string election_prefix_;
  std::string participant_key_;
  std::uint64_t participant_revision_;

  mutable std::mutex mu_;
  std::condition_variable cv_;
  state state_;
  std::set<std::uint64_t> current_watches_;
  std::set<std::string> watched_keys_;
  int pending_async_ops_;

  std::promise<bool> campaign_result_;
  std::function<void(std::future<bool>&)> campaign_callback_;
};

} // namespace etcd
} // namespace jb

#endif // jb_etcd_leader_election_participant_hpp
