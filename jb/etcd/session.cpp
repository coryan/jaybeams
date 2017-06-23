#include "jb/etcd/session.hpp"
#include <jb/assert_throw.hpp>
#include <jb/log.hpp>

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace jb {
namespace etcd {

int constexpr session::keep_alives_per_ttl;

void session::initiate_shutdown() {
  {
    // ... new scope because we do not want to hold the mutex when
    // calling WritesDone() ...
    std::lock_guard<std::mutex> lock(mu_);
    if (state_ == state::shuttingdown or state_ == state::shutdown) {
      return;
    }
    // ... stop any new timers from being created ...
    state_ = state::shuttingdown;
    // ... cancel any outstanding timers ...
    alarm_->Cancel();
  }
  // ... half-close the reader-writer stream ...
  rdwr_->WritesDone(&tag_on_writes_done_);
}

void session::run() try {
  // This function is wrapped in try/catch block because it runs in
  // its own thread.  If we let exceptions scape the program would
  // terminate.
  // TODO() - maybe we should let the program terminate in that
  // case, the application would lose the master election.  I
  // believe that should be a matter of policy, so for the moment
  // log ...
  void* tag = nullptr;
  bool ok = false;
  while (queue_.Next(&tag, &ok)) {
    if (not ok or tag == nullptr) {
      // TODO() - I think tag == nullptr should never happen,
      // consider using JB_ASSERT()
      continue;
    }
    // ... tag must be a pointer to std::function<void()> see the
    // comments for tag_set_timer_ and friends to understand why ...
    auto callback = static_cast<std::function<void()>*>(tag);
    (*callback)();
  }
  // ... finally done, change the state ...
  std::lock_guard<std::mutex> lock(mu_);
  JB_ASSERT_THROW(state_ == state::shuttingdown);
  state_ = state::shutdown;
} catch (std::exception const& ex) {
  JB_LOG(info) << "Standard C++ exception in session::run(): " << ex.what();
} catch (...) {
  JB_LOG(info) << "Unknown exception in session::run()";
}

session::session(
    std::shared_ptr<grpc::Channel> etcd_service,
    std::chrono::milliseconds desired_TTL, bool)
    : mu_()
    , state_(state::constructing)
    , etcd_service_(etcd_service)
    , lease_id_(0)
    , desired_TTL_(desired_TTL)
    , actual_TTL_(desired_TTL)
    , queue_()
    , stub_(etcdserverpb::Lease::NewStub(etcd_service_))
    , kactx_()
    , rdwr_()
    , alarm_()
    , ka_req_()
    , ka_res_()
    , finish_status_()
    , tag_set_timer_(make_tag(&session::set_timer))
    , tag_on_timeout_(make_tag(&session::on_timeout))
    , tag_on_write_(make_tag(&session::on_write))
    , tag_on_read_(make_tag(&session::on_read))
    , tag_on_writes_done_(make_tag(&session::on_writes_done))
    , tag_on_finish_(make_tag(&session::on_finish)) {
  // ...request a new lease from etcd(1) ...
  etcdserverpb::LeaseGrantRequest req;
  // ... the TTL is is seconds, convert to the right units ...
  auto ttl_seconds =
      std::chrono::duration_cast<std::chrono::seconds>(desired_TTL_);
  req.set_ttl(ttl_seconds.count());
  // ... let the etcd(1) pick a lease ID ...
  req.set_id(0);

  // TODO() - I think we should set a timeout here ... no operation
  // should be allowed to block forever ...
  grpc::ClientContext context;
  etcdserverpb::LeaseGrantResponse resp;
  auto status = stub_->LeaseGrant(&context, req, &resp);
  if (not status.ok()) {
    std::ostringstream os;
    os << "stub->LeaseGrant() failed: " << status.error_message() << "["
       << status.error_code() << "]";
    // TODO() - do we want to have more advanced policies on how to
    // recover from this errors?  And if so, why?
    throw std::runtime_error(os.str());
  }

  // TODO() - probably need to retry until it succeeds, with some
  // kind of backoff, and yes, a really, really long timeout ...
  if (resp.error() != "") {
    std::ostringstream os;
    // TODO() - use the typical protobuf formatting here ...
    os << "Lease grant request rejected\n"
       << "    header.cluster_id=" << resp.header().cluster_id() << "\n"
       << "    header.member_id=" << resp.header().member_id() << "\n"
       << "    header.revision=" << resp.header().revision() << "\n"
       << "    header.raft_term=" << resp.header().raft_term() << "\n"
       << "  error=" << resp.error();
    throw std::runtime_error(os.str());
  }

  JB_LOG(info) << "Lease granted\n"
               << "    header.cluster_id=" << resp.header().cluster_id() << "\n"
               << "    header.member_id=" << resp.header().member_id() << "\n"
               << "    header.revision=" << resp.header().revision() << "\n"
               << "    header.raft_term=" << resp.header().raft_term() << "\n"
               << "  ID=" << std::hex << std::setw(16) << std::setfill('0')
               << resp.id() << "  TTL=" << std::dec << resp.ttl() << "s\n";
  lease_id_ = resp.id();
  actual_TTL_ = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::seconds(resp.ttl()));

  // ... no real need to grab a mutex here.  The object is not fully
  // constructed, it should not be used by more than one thread ...
  state_ = state::connecting;

  // ... create the reader-writer to send KeepAlive requests and
  // receive the corresponding responses, call set_timer() when done
  // the reader/writer is ready ...
  rdwr_ = stub_->AsyncLeaseKeepAlive(&kactx_, &queue_, &tag_set_timer_);
}

/// Set a timer to start the next Write/Read cycle.
void session::set_timer() {
  // TODO() - need a mutex here ...
  if (state_ == state::shuttingdown or state_ == state::shutdown) {
    return;
  }
  state_ = state::connecting;
  // ... use the awkward grpc::Alarm interface to completion queues,
  // and once that is done, call on_timer().  I was tempted to set
  // the timer as soon as the write operation was scheduled, but the
  // AsyncReaderWriter docs says you can only have one outstanding
  // Write() request at a time.  If we started the timer there is no
  // guarantee that the timer won't expire before the next
  // response.  Notice that gRPC++ uses deadlines (not timeouts) for
  // its timers, and that they are based on
  // std::chrono::system_clock.  That clock is not guaranteed to be
  // monotonic, so not a good choice in my opinion, meh ...
  auto deadline =
      std::chrono::system_clock::now() + (actual_TTL_ / keep_alives_per_ttl);
  alarm_.reset(new grpc::Alarm(&queue_, deadline, &tag_on_timeout_));
}

/// Handle the timer expiration, Write() a KeepAlive request.
void session::on_timeout() {
  ka_req_.set_id(lease_id_);
  rdwr_->Write(ka_req_, static_cast<void*>(&tag_on_write_));
}

/// Handle the Write() completion, schedule a new Read().
void session::on_write() {
  rdwr_->Read(&ka_res_, static_cast<void*>(&tag_on_read_));
}

/// Handle the Read() completion, schedule a new Timer().
void session::on_read() {
  // ... the KeepAliveResponse may have a new TTL value, that is the
  // etcd server may be telling us to backoff a little ...
  actual_TTL_ = std::chrono::seconds(ka_res_.ttl());
  // TODO() - remove debugging log ...
  JB_LOG(debug) << "reset timer to " << actual_TTL_.count() << "ms"
                << ", received=" << ka_res_.ttl() << "s"
                << ", desired=" << desired_TTL_.count() << "ms";
  set_timer();
}

/// Handle the WritesDone() completion, schedule a Finish()
void session::on_writes_done() {
  rdwr_->Finish(&finish_status_, &tag_on_finish_);
}

/// Handle the Finish() completion, schedule a LeaseRevoke()
void session::on_finish() {
  if (not finish_status_.ok()) {
    JB_LOG(info) << "on the session for lease=" << lease_id_
                 << " the rdwr->Finish() op failed: "
                 << finish_status_.error_message() << "["
                 << finish_status_.error_code() << "]";
  }

  // ... here we just block, we could make this asynchronous, but
  // really there is no reason to.  This is running in the session's
  // thread, and there is no more use for the completion queue, no
  // pending operations or anything ...
  grpc::ClientContext context;
  etcdserverpb::LeaseRevokeRequest req;
  req.set_id(lease_id_);
  etcdserverpb::LeaseRevokeResponse resp;
  auto status = stub_->LeaseRevoke(&context, req, &resp);
  if (not status.ok()) {
    std::ostringstream os;
    JB_LOG(info) << "stub->LeaseRevoke() failed for lease=" << std::hex
                 << std::setw(16) << std::setfill('0') << lease_id_ << ": "
                 << status.error_message() << "[" << status.error_code() << "]";
  } else {
    JB_LOG(info) << "Lease Revoked\n"
                 << "    header.cluster_id=" << resp.header().cluster_id()
                 << "\n"
                 << "    header.member_id=" << resp.header().member_id() << "\n"
                 << "    header.revision=" << resp.header().revision() << "\n"
                 << "    header.raft_term=" << resp.header().raft_term() << "\n"
                 << "  id=" << std::hex << std::setw(16) << std::setfill('0')
                 << lease_id_ << "\n";
  }
  queue_.Shutdown();
}

std::function<void()> session::make_tag(callback_member member) {
  return std::function<void()>([this, member]() { (this->*member)(); });
}

} // namespace etcd
} // namespace jb
