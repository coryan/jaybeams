#ifndef jb_etcd_leader_election_participant_hpp
#define jb_etcd_leader_election_participant_hpp

#include <jb/etcd/client_factory.hpp>
#include <jb/etcd/session.hpp>
#include <jb/etcd/completion_queue.hpp>

#include <atomic>
#include <future>

namespace jb {
namespace etcd {

/**
 * Participate in a leader election protocol.
 */
class leader_election_participant {
public:
  /// Constructor, blocks until this participant becomes the leader.
  leader_election_participant(
      std::shared_ptr<client_factory> client_factory,
      std::string const& etcd_endpoint, std::string const& election_name,
      std::string const& participant_value)
      : leader_election_participant(
            true, client_factory, etcd_endpoint, election_name,
            participant_value) {
    // ... block until this participant becomes the leader ...
    campaign();
  }

  /// Constructor, non-blocking, calls the callback when elected.
  template <typename Functor>
  leader_election_participant(
      std::shared_ptr<client_factory> client_factory,
      std::string const& etcd_endpoint, std::string const& election_name,
      std::string const& participant_value, Functor const& elected_callback)
      : leader_election_participant(
                                    true, client_factory, etcd_endpoint, election_name, participant_value,
            std::move(elected_callback)) {
  }

  /// Constructor, non-blocking, calls the callback when elected.
  template <typename Functor>
  leader_election_participant(
      std::shared_ptr<client_factory> client_factory,
      std::string const& etcd_endpoint, std::string const& election_name,
      std::string const& participant_value, Functor&& elected_callback)
      : leader_election_participant(
            true, client_factory, etcd_endpoint, election_name,
            participant_value) {
    campaign(elected_callback);
  }

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
      bool shared, std::shared_ptr<client_factory> client_factory,
      std::string const& etcd_endpoint, std::string const& election_name,
      std::string const& participant_value);

  /**
   * Runs the operations before starting the election campaign.
   *
   * This function can throw exceptions which means the campaign was
   * never even started.
   */
  void preamble();

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

  using range_predecessor_op = async_op<etcdserverpb::RangeResponse>;
  void on_range_request(std::shared_ptr<range_predecessor_op> op);

  using watch_write_op = async_write_op<etcdserverpb::WatchRequest>;
  using watch_read_op = async_read_op<etcdserverpb::WatchResponse>;

  void on_watch_write(
      std::shared_ptr<watch_write_op> op,
      std::string const& watched_key, std::uint64_t watched_revision);
  void on_watch_read(std::shared_ptr<watch_read_op> op);

  /// Check if the election has finished, if so invoke the callbacks.
  void check_election_over_maybe();

  /// Run the completion queue event loop.  Called in a separate
  /// thread ...
  void run();

  // Invoke the callback, notice that the callback is invoked only once.
  void make_callback();

  /// Log an exception caught when the event loop exits
  void log_thread_exit_exception(std::exception_ptr eptr);

  /// Return an exception with the @a status details
  std::runtime_error
  error_grpc_status(char const* where, grpc::Status const& status) const;

  // Compute the end of the range for a prefix search.
  static std::string prefix_end(std::string const& prefix);

private:
  std::shared_ptr<client_factory> client_;
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<etcdserverpb::KV::Stub> kv_client_;
  std::unique_ptr<etcdserverpb::Watch::Stub> watch_client_;
  using watcher_stream_type = grpc::ClientAsyncReaderWriter<
    etcdserverpb::WatchRequest, etcdserverpb::WatchResponse>;
  std::unique_ptr<watcher_stream_type> watcher_stream_;
  std::shared_ptr<completion_queue> queue_;
  session session_;
  std::string election_name_;
  std::string participant_value_;
  std::string election_prefix_;
  std::string participant_key_;
  std::uint64_t participant_revision_;

  mutable std::mutex mu_;
  std::atomic<std::int64_t> pending_watches_;
  std::promise<bool> campaign_result_;
  std::function<void(std::future<bool>&)> campaign_callback_;

  std::thread completion_queue_loop_;
};

} // namespace etcd
} // namespace jb

#endif // jb_etcd_leader_election_participant_hpp
