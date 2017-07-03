#ifndef jb_etcd_leader_election_participant_hpp
#define jb_etcd_leader_election_participant_hpp

#include <jb/etcd/active_completion_queue.hpp>
#include <jb/etcd/leader_election_runner.hpp>
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
      std::shared_ptr<grpc::Channel> etcd_channel,
      std::string const& election_name, std::string const& participant_value,
      duration_type d, std::uint64_t lease_id = 0)
      : leader_election_participant(
            true, queue, etcd_channel, election_name, participant_value,
            session::convert_duration(d), lease_id) {
    // ... block until this participant becomes the leader ...
    campaign();
  }

  /// Constructor, non-blocking, calls the callback when elected.
  template <typename duration_type, typename Functor>
  leader_election_participant(
      std::shared_ptr<active_completion_queue> queue,
      std::shared_ptr<grpc::Channel> etcd_channel,
      std::string const& election_name, std::string const& participant_value,
      Functor const& elected_callback, duration_type d,
      std::uint64_t lease_id = 0)
      : leader_election_participant(
            true, queue, etcd_channel, election_name, participant_value,
            session::convert_duration(d), lease_id) {
    // ... adapt the functor and then call campaign_impl ...
    campaign(std::move(elected_callback));
  }

  /// Constructor, non-blocking, calls the callback when elected.
  template <typename duration_type, typename Functor>
  leader_election_participant(
      std::shared_ptr<active_completion_queue> queue,
      std::shared_ptr<grpc::Channel> etcd_channel,
      std::string const& election_name, std::string const& participant_value,
      Functor&& elected_callback, duration_type d, std::uint64_t lease_id = 0)
      : leader_election_participant(
            true, queue, etcd_channel, election_name, participant_value,
            session::convert_duration(d), lease_id) {
    // ... adapt the functor and then call campaign_impl ...
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

  /// Return the etcd key associated with this participant
  std::string const& key() const {
    return runner_->key();
  }

  /// Return the etcd eky associated with this participant
  std::string const& value() const {
    return runner_->value();
  }

  /// Return the fetched participant revision, mostly for debugging
  std::uint64_t participant_revision() const {
    return runner_->participant_revision();
  }

  /// Return the lease corresponding to this participant's session.
  std::uint64_t lease_id() const {
    return runner_->lease_id();
  }

  /// Resign from the election, terminate the internal loops
  void resign();

  /// Change the published value
  void proclaim(std::string const& new_value);

private:
  /// Refactor common code to public constructors ...
  leader_election_participant(
      bool, std::shared_ptr<active_completion_queue> queue,
      std::shared_ptr<grpc::Channel>, std::string const& election_name,
      std::string const& participant_value, session::duration_type desired_TTL,
      std::uint64_t lease_id);

  /// Block the calling thread until the participant has become the
  /// leader.
  void campaign();

  /**
   * Refactor template code via std::function
   *
   * The main "interface" is the campaing() template member functions,
   * but we loath duplicating that much code here, so refactor with a
   * std::function<>.  The cost of such functions is higher, but
   * leader election is not a fast operation.
   */
  void campaign_impl(std::function<void(bool)>&& callback);

  /// Adaptor for generic functors ...
  template <typename Functor>
  void campaign(Functor&& callback) {
    campaign_impl(std::function<void(bool)>(std::move(callback)));
  }

private:
  std::shared_ptr<active_completion_queue> queue_;
  std::shared_ptr<grpc::Channel> channel_;
  std::shared_ptr<session> session_;
  std::shared_ptr<leader_election_runner> runner_;
  std::string election_name_;
  std::string initial_value_;
};

} // namespace etcd
} // namespace jb

#endif // jb_etcd_leader_election_participant_hpp
