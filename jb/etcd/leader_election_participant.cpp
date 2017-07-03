#include "jb/etcd/leader_election_participant.hpp"
#include <jb/etcd/detail/leader_election_runner_impl.hpp>
#include <jb/etcd/detail/session_impl.hpp>
#include <jb/etcd/grpc_errors.hpp>
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
    session::duration_type desired_TTL, std::uint64_t lease_id)
    : queue_(queue)
    , channel_(etcd_channel)
    , session_(new detail::session_impl<completion_queue<>>(
          queue->cq(), etcdserverpb::Lease::NewStub(channel_), desired_TTL,
          lease_id))
    , runner_()
    , election_name_(election_name)
    , initial_value_(participant_value) {
}

leader_election_participant::~leader_election_participant() noexcept(false) {
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
    // ... the callback receives a bool, hopefully with "true"
    // indicating the election was successful.  Setting the value
    // could raise, but that only happens when
    if (result) {
      elected.set_value(result);
    } else {
      std::ostringstream os;
      os << "election aborted for " << std::hex << this->key();
      elected.set_exception(
          std::make_exception_ptr(std::runtime_error(os.str())));
    }
  });
  // ... block until the promise is satisfied, notice that get()
  // raises the exception if that was the result ...
  JB_LOG(trace) << key() << "  blocked running election";
  elected.get_future().get();
}

void leader_election_participant::campaign_impl(
    std::function<void(bool)>&& callback) {
  runner_.reset(new detail::leader_election_runner_impl<completion_queue<>>(
      queue_->cq(), session_->lease_id(), etcdserverpb::KV::NewStub(channel_),
      etcdserverpb::Watch::NewStub(channel_), std::move(election_name_),
      std::move(initial_value_), std::move(callback)));
}

} // namespace etcd
} // namespace jb
