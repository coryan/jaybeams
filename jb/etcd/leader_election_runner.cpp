#include "jb/etcd/leader_election_runner.hpp"

namespace jb {
namespace etcd {

leader_election_runner_base::leader_election_runner_base(
    std::uint64_t lease_id, std::unique_ptr<etcdserverpb::KV::Stub> kv_client,
    std::unique_ptr<etcdserverpb::Watch::Stub> watch_client,
    std::string const& election_name, std::string const& participant_value)
    : mu_()
    , cv_()
    , state_(state::constructing)
    , pending_async_ops_(0)
    , election_name_(election_name)
    , participant_value_(participant_value)
    , election_prefix_(election_name + "/")
    , participant_key_([election_name, lease_id]() {
      // ... build the key using the session's lease.  The lease id is
      // unique, assigned by etcd, so each participant gets a different
      // key.  The use of an inline functor is because why not ...
      // TODO() - document why there can be no accidental conflict with
      // another election running in the system.
      std::ostringstream os;
      os << election_name << "/" << std::hex << lease_id;
      return os.str();
    }())
    , lease_id_(lease_id)
    , kv_client_(std::move(kv_client))
    , watch_client_(std::move(watch_client))
    , watcher_stream_() {
}

leader_election_runner_base::~leader_election_runner_base() noexcept(false) {
}

std::string leader_election_runner_base::log_header(char const* log) const {
  std::ostringstream os;
  os << key() << " " << state_ << " " << pending_async_ops_ << log;
  return os.str();
}

void leader_election_runner_base::async_ops_block() {
  std::unique_lock<std::mutex> lock(mu_);
  cv_.wait(lock, [this]() { return pending_async_ops_ == 0; });
}

bool leader_election_runner_base::async_op_start(char const* msg) {
  JB_LOG(info) << log_header("    ") << msg;
  std::unique_lock<std::mutex> lock(mu_);
  if (state_ == state::shuttingdown or state_ == state::shutdown) {
    return false;
  }
  ++pending_async_ops_;
  return true;
}

bool leader_election_runner_base::async_op_start_shutdown(char const* msg) {
  JB_LOG(info) << log_header("    ") << msg << " during shutdown";
  std::unique_lock<std::mutex> lock(mu_);
  ++pending_async_ops_;
  return true;
}

void leader_election_runner_base::async_op_done(char const* msg) {
  JB_LOG(info) << log_header("      ") << msg;
  std::unique_lock<std::mutex> lock(mu_);
  if (--pending_async_ops_ == 0) {
    lock.unlock();
    cv_.notify_one();
    return;
  }
}

bool leader_election_runner_base::set_state(char const* msg, state new_state) {
  std::lock_guard<std::mutex> lock(mu_);
  JB_LOG(info) << log_header("      ") << msg << " " << new_state;
  if (state_ == state::shuttingdown or state_ == state::shutdown) {
    return false;
  }
  state_ = new_state;
  return true;
}

char const* to_str(leader_election_state s) {
  static char const* names[] = {
      "constructing", "connecting", "testandset",   "republish",
      "published",    "querying",   "campaigning",  "elected",
      "resigning",    "resigned",   "shuttingdown", "shutdown",
  };
  JB_ASSERT_THROW(
      0 <= int(s) and std::size_t(s) < sizeof(names) / sizeof(names[0]));
  return names[int(s)];
}

std::ostream& operator<<(std::ostream& os, leader_election_state x) {
  return os << to_str(x);
}

} // namespace etcd
} // namespace jb
