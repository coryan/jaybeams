#include "jb/etcd/leader_election_participant.hpp"
#include <jb/etcd/grpc_errors.hpp>
#include <jb/etcd/leader_election_runner.hpp>
#include <jb/etcd/log_promise_errors.hpp>
#include <jb/etcd/prefix_end.hpp>
#include <jb/assert_throw.hpp>
#include <jb/log.hpp>

#include <google/protobuf/text_format.h>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace jb {
namespace etcd {

// Does not initialize "runner", that is left for the
// campaign_impl() function, which is called by all constructors ...
leader_election_participant::leader_election_participant(
    bool shared, std::shared_ptr<active_completion_queue> queue,
    std::shared_ptr<grpc::Channel> etcd_channel,
    std::string const& election_name, std::string const& participant_value,
    std::shared_ptr<session> session)
    : queue_(queue)
    , channel_(etcd_channel)
    , session_(session)
    , runner_()
    , election_name_(election_name)
    , initial_value_(participant_value) {
}

leader_election_participant::~leader_election_participant() noexcept(false) {
}

/// Return the etcd key associated with this participant
std::string const& leader_election_participant::key() const {
  return runner_->key();
}

/// Return the etcd eky associated with this participant
std::string const& leader_election_participant::value() const {
  return runner_->value();
}

/// Return the fetched participant revision, mostly for debugging
std::uint64_t leader_election_participant::participant_revision() const {
  return runner_->participant_revision();
}

/// Return the lease corresponding to this participant's session.
std::uint64_t leader_election_participant::lease_id() const {
  return runner_->lease_id();
}

void leader_election_participant::resign() {
  session_->revoke();
  runner_->resign();
}

void leader_election_participant::proclaim(std::string const& new_value) {
  runner_->proclaim(new_value);
}

void leader_election_participant::campaign() {
  // We are going to wait using a local promise ...
  std::promise<bool> elected;
  // ... this will kick-off the campaign, a separate thread runs the
  // asynchronous operations to wait until the campaign is done.  It
  // will call the provided lambda when it is done ...
  campaign([&elected, this](bool result) {
    // ... the callback receives a future, hopefully with "true"
    // indicating the election was successful, but it could be an
    // exception.  We capture both case ..
    try {
      elected.set_value(result);
    } catch (...) {
      JB_LOG(info) << this->key() << " failed to set election result";
      // elected.set_exception(std::current_exception());
    }
  });
  // ... block until the promise is satisfied, notice that get()
  // raises the exception if that was the result ...
  JB_LOG(trace) << key() << "  blocked running election";
  if (elected.get_future().get() != true) {
    // ... we also raise if the campaign failed ...
    std::ostringstream os;
    os << "Unexpected false value after running campaign, key=" << key();
    throw std::runtime_error(os.str());
  }
}

void leader_election_participant::campaign_impl(
    std::function<void(bool)>&& callback) {
  runner_.reset(new leader_election_runner<completion_queue<>>(
      queue_->cq(), session_->lease_id(), etcdserverpb::KV::NewStub(channel_),
      etcdserverpb::Watch::NewStub(channel_), std::move(election_name_),
      std::move(initial_value_), std::move(callback)));
}

} // namespace etcd
} // namespace jb
