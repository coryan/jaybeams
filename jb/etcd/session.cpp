#include "jb/etcd/session.hpp"
#include <jb/etcd/active_completion_queue.hpp>
#include <jb/assert_throw.hpp>
#include <jb/log.hpp>

#include <google/protobuf/text_format.h>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace jb {
namespace etcd {

int constexpr session::keep_alives_per_ttl;

session::~session() noexcept(false) {
  shutdown();
}

void session::revoke() {
  // ... here we just block, we could make this asynchronous, but
  // really there is no reason to.  This is running in the session's
  // thread, and there is no more use for the completion queue, no
  // pending operations or anything ...
  grpc::ClientContext context;
  etcdserverpb::LeaseRevokeRequest req;
  req.set_id(lease_id_);
  etcdserverpb::LeaseRevokeResponse resp;
  auto status = lease_client_->LeaseRevoke(&context, req, &resp);
  if (not status.ok()) {
    throw error_grpc_status("LeaseRevoke()", status, &resp);
  }
  JB_LOG(info) << std::hex << lease_id() << " lease Revoked";
  shutdown();
}

session::session(
    bool, std::shared_ptr<active_completion_queue> queue,
    std::shared_ptr<client_factory> client, std::string const& etcd_endpoint,
    std::chrono::milliseconds desired_TTL, std::uint64_t lease_id)
    : mu_()
    , state_(state::constructing)
    , client_(client)
    , channel_(client_->create_channel(etcd_endpoint))
    , lease_client_(client_->create_lease(channel_))
    , keep_alive_stream_context_()
    , keep_alive_stream_()
    , queue_(queue)
    , lease_id_(lease_id)
    , desired_TTL_(desired_TTL)
    , actual_TTL_(desired_TTL) {
  preamble();
}

void session::preamble() try {
  // ...request a new lease from the etcd server ...
  etcdserverpb::LeaseGrantRequest req;
  // ... the TTL is is seconds, convert to the right units ...
  auto ttl_seconds =
      std::chrono::duration_cast<std::chrono::seconds>(desired_TTL_);
  req.set_ttl(ttl_seconds.count());
  req.set_id(lease_id_);

  // TODO() - I think we should set a timeout here ... no operation
  // should be allowed to block forever ...
  grpc::ClientContext context;
  etcdserverpb::LeaseGrantResponse resp;
  auto status = lease_client_->LeaseGrant(&context, req, &resp);
  if (not status.ok()) {
    throw error_grpc_status("LeaseGrant()", status, &resp);
  }

  // TODO() - probably need to retry until it succeeds, with some
  // kind of backoff, and yes, a really, really long timeout ...
  if (resp.error() != "") {
    std::ostringstream os;
    os << "Lease grant request rejected\n";
    std::string print;
    google::protobuf::TextFormat::PrintToString(req, &print);
    os << "request=" << print;
    print.clear();
    google::protobuf::TextFormat::PrintToString(resp, &print);
    os << "\nresponse=" << print;
    throw std::runtime_error(os.str());
  }

  lease_id_ = resp.id();
  JB_LOG(info) << std::hex << lease_id() << " - lease granted"
               << "  TTL=" << std::dec << resp.ttl() << "s";
  actual_TTL_ = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::seconds(resp.ttl()));

  // ... no real need to grab a mutex here.  The object is not fully
  // constructed, it should not be used by more than one thread ...
  state_ = state::connecting;

  // ... we want to block until the keep alive streaming RPC is setup,
  // this is (unfortunately) an asynchronous operation, so we have to
  // do some magic ...
  std::promise<bool> stream_ready;
  auto op = make_async_rdwr_stream<
      ka_stream_type::write_type, ka_stream_type::read_type>(
      [&stream_ready](auto op) { stream_ready.set_value(true); });
  // ... create the call and invoke the operation's callback when done ...
  keep_alive_stream_ = lease_client_->AsyncLeaseKeepAlive(
      &keep_alive_stream_context_, *queue_, op->tag());
  // ... block until done ...
  if (not stream_ready.get_future().get()) {
    JB_LOG(error) << std::hex << lease_id() << " stream not ready!!";
  }

  JB_LOG(info) << std::hex << lease_id() << " stream connected";

  state_ = state::connected;
  set_timer();
} catch (std::exception const& ex) {
  JB_LOG(info) << std::hex << lease_id()
               << " std::exception raised in preamble: " << ex.what();
  shutdown();
  throw;
} catch (...) {
  JB_LOG(info) << std::hex << lease_id()
               << " unknown exception raised in preamble";
  shutdown();
  throw;
}

void session::shutdown() {
  {
    // This stops new timers (and therefore any other operations) from
    // beign created ...
    std::lock_guard<std::mutex> lock(mu_);
    if (state_ == state::shuttingdown or state_ == state::shutdown) {
      return;
    }
    state_ = state::shuttingdown;
    // ... only one thread gets past this point ...
  }
  // ... cancel the outstanding timer, if any ...
  if (current_timer_) {
    current_timer_->cancel();
    current_timer_.reset();
  }
  if (keep_alive_stream_) {
    // The KeepAlive stream was already created, we need to close it
    // before shutting down ...
    std::promise<bool> stream_closed;
    auto op = make_writes_done_op([this, &stream_closed](auto op) {
      this->on_writes_done(op, stream_closed);
    });
    keep_alive_stream_->WritesDone(op->tag());
    // ... block until it closes ...
    stream_closed.get_future().get();
  }
  std::lock_guard<std::mutex> lock(mu_);
  state_ = state::shutdown;
}

void session::set_timer() {
  {
    std::lock_guard<std::mutex> lock(mu_);
    if (state_ != state::connected) {
      return;
    }
  }
  // ... we are going to schedule a new timer, in general, we should
  // only schedule a timer when there are no pending KeepAlive
  // request/responses in the stream.  The AsyncReaderWriter docs says
  // you can only have one outstanding Write() request at a time.  If
  // we started the timer there is no guarantee that the timer won't
  // expire before the next response.

  auto deadline =
      std::chrono::system_clock::now() + (actual_TTL_ / keep_alives_per_ttl);
  current_timer_ = queue_->cq().make_deadline_timer(
      deadline, [this](auto op) { this->on_timeout(op); });
}

void session::on_timeout(std::shared_ptr<deadline_timer> op) {
  {
    std::lock_guard<std::mutex> lock(mu_);
    if (state_ == state::shuttingdown or state_ == state::shutdown) {
      return;
    }
  }
  auto write = make_write_op<etcdserverpb::LeaseKeepAliveRequest>(
      [this](auto op) { this->on_write(op); });
  write->request.set_id(lease_id());
  keep_alive_stream_->Write(write->request, write->tag());
}

void session::on_write(std::shared_ptr<ka_stream_type::write_op> op) {
  {
    std::lock_guard<std::mutex> lock(mu_);
    if (state_ == state::shuttingdown or state_ == state::shutdown) {
      return;
    }
  }
  auto read = make_read_op<etcdserverpb::LeaseKeepAliveResponse>(
      [this](auto rd) { this->on_read(rd); });
  this->keep_alive_stream_->Read(&read->response, read->tag());
}

void session::on_read(std::shared_ptr<ka_stream_type::read_op> op) {
  {
    std::lock_guard<std::mutex> lock(mu_);
    if (state_ == state::shuttingdown or state_ == state::shutdown) {
      return;
    }
  }
  // ... the KeepAliveResponse may have a new TTL value, that is the
  // etcd server may be telling us to backoff a little ...
  actual_TTL_ = std::chrono::seconds(op->response.ttl());
  set_timer();
}

void session::on_writes_done(
    std::shared_ptr<writes_done_op> writes_done, std::promise<bool>& done) {
  auto op =
      make_finish_op([this, &done](auto op) { this->on_finish(op, done); });
  keep_alive_stream_->Finish(&op->status, op->tag());
  JB_LOG(info) << std::hex << lease_id()
               << " finish scheduled, status=" << op->status.error_message()
               << " [" << op->status.error_code() << "]";
}

void session::on_finish(
    std::shared_ptr<finish_op> op, std::promise<bool>& done) {
  try {
    if (not op->status.ok()) {
      throw error_grpc_status("on_finish", op->status);
    }
    done.set_value(true);
  } catch (...) {
    try {
      done.set_exception(std::current_exception());
    } catch (std::future_error const& ex) {
      JB_LOG(info) << std::hex << lease_id()
                   << " std::future_error raised while reporting "
                   << "exception in lease keep alive stream closure: "
                   << ex.what();
    } catch (std::exception const& ex) {
      JB_LOG(info) << std::hex << lease_id()
                   << " std::exception raised while reporting "
                   << "exception in lease keep alive stream closure: "
                   << ex.what();
    } catch (...) {
      JB_LOG(info) << std::hex << lease_id()
                   << " unknown exception raised while reporting "
                   << "exception in lease keep alive stream closure.";
    }
  }
}

std::runtime_error session::error_grpc_status(
    char const* where, grpc::Status const& status,
    google::protobuf::Message const* res,
    google::protobuf::Message const* req) const {
  std::ostringstream os;
  os << "grpc error in " << where << " for lease=" << lease_id() << ": "
     << status.error_message() << "[" << status.error_code() << "]";
  if (res != nullptr) {
    std::string print;
    google::protobuf::TextFormat::PrintToString(*res, &print);
    os << ", response=" << print;
  }
  if (req != nullptr) {
    std::string print;
    google::protobuf::TextFormat::PrintToString(*req, &print);
    os << ", request=" << print;
  }
  return std::runtime_error(os.str());
}

} // namespace etcd
} // namespace jb
