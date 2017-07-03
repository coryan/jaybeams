#ifndef jb_detail_etcd_session_impl_hpp
#define jb_detail_etcd_session_impl_hpp

#include <jb/etcd/grpc_errors.hpp>
#include <jb/etcd/log_promise_errors.hpp>
#include <jb/etcd/session.hpp>
#include <jb/log.hpp>

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace jb {
namespace etcd {
namespace detail {

template <typename completion_queue_type>
class session_impl : public session {
public:
  /// Constructor
  template <typename duration_type>
  session_impl(
      completion_queue_type& queue,
      std::unique_ptr<etcdserverpb::Lease::Stub> lease_stub,
      duration_type desired_TTL)
      : session_impl(
            queue, std::move(lease_stub), convert_duration(desired_TTL), 0) {
  }

  /**
   * Contructor with a previous lease.
   *
   * This could be useful for an application that saves its lease, shuts
   * down, and quickly restarts.  I think it is a stretch, but so easy
   * to implement that why not?
   */
  template <typename duration_type>
  session_impl(
      completion_queue_type& queue,
      std::unique_ptr<etcdserverpb::Lease::Stub> lease_stub,
      duration_type desired_TTL, std::uint64_t lease_id)
      : session_impl(
            true, queue, std::move(lease_stub), convert_duration(desired_TTL),
            lease_id) {
  }

  virtual ~session_impl() {
    shutdown();
  }

  /// Revoke the lease
  virtual void revoke() override {
    // ... here we just block, we could make this asynchronous, but
    // really there is no reason to.  This is running in the session's
    // thread, and there is no more use for the completion queue, no
    // pending operations or anything ...
    grpc::ClientContext context;
    etcdserverpb::LeaseRevokeRequest req;
    req.set_id(lease_id_);
    etcdserverpb::LeaseRevokeResponse resp;
    auto status = lease_client_->LeaseRevoke(&context, req, &resp);
    check_grpc_status(
        status, "session::LeaseRevoke()", " lease_id=", lease_id(),
        ", response=", jb::etcd::print_to_stream(resp));
    JB_LOG(trace) << std::hex << lease_id() << " lease Revoked";
    shutdown();
  }

private:
  /// Common constructor
  session_impl(
      bool, completion_queue_type& queue,
      std::unique_ptr<etcdserverpb::Lease::Stub> lease_stub,
      std::chrono::milliseconds desired_TTL, std::uint64_t lease_id)
      : session(std::move(lease_stub), desired_TTL, lease_id)
      , queue_(queue) {
    preamble();
  }

  /// Requests (or renews) the lease and setup the watcher stream.
  void preamble() try {
    // ...request a new lease from the etcd server ...
    etcdserverpb::LeaseGrantRequest req;
    // ... the TTL is is seconds, convert to the right units ...
    auto ttl_seconds =
        std::chrono::duration_cast<std::chrono::seconds>(desired_TTL_);
    req.set_ttl(ttl_seconds.count());
    req.set_id(lease_id_);

    auto lfut = queue_.async_rpc(
        lease_client_.get(), &etcdserverpb::Lease::Stub::AsyncLeaseGrant,
        std::move(req), "session/preamble/lease_grant", jb::etcd::use_future());
    auto resp = lfut.get();

    // TODO() - probably need to retry until it succeeds, with some
    // kind of backoff, and yes, a really, really long timeout ...
    if (resp.error() != "") {
      std::ostringstream os;
      os << "Lease grant request rejected"
         << "\n request=" << print_to_stream(req) << "\n response"
         << print_to_stream(resp);
      throw std::runtime_error(os.str());
    }

    lease_id_ = resp.id();
    JB_LOG(trace) << std::hex << lease_id() << " - lease granted"
                  << "  TTL=" << std::dec << resp.ttl() << "s";
    actual_TTL_ = convert_duration(std::chrono::seconds(resp.ttl()));

    // ... no real need to grab a mutex here.  The object is not fully
    // constructed, it should not be used by more than one thread ...
    state_ = state::connecting;

    // ... we want to block until the keep alive streaming RPC is setup,
    // this is (unfortunately) an asynchronous operation, so we have to
    // do some magic ...
    std::promise<bool> stream_ready;
    auto fut = queue_.async_create_rdwr_stream(
        lease_client_.get(), &etcdserverpb::Lease::Stub::AsyncLeaseKeepAlive,
        "session/ka_stream", jb::etcd::use_future());
    this->ka_stream_ = fut.get();

    JB_LOG(trace) << std::hex << lease_id() << " stream connected";
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

  /// Shutdown the local resources.
  void shutdown() {
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
    if (ka_stream_) {
      // The KeepAlive stream was already created, we need to close it
      // before shutting down ...
      auto writes_done_complete = queue_.async_writes_done(
          *ka_stream_, "session/shutdown/writes_done", jb::etcd::use_future());
      // ... block until it closes ...
      writes_done_complete.get();

      auto finish_complete = queue_.async_finish(
          *ka_stream_, "session/ka_stream/finish", jb::etcd::use_future());
      auto status = finish_complete.get();
      check_grpc_status(status, "session::finish()");
    }
    std::lock_guard<std::mutex> lock(mu_);
    state_ = state::shutdown;
  }

  /// Set a timer to start the next Write/Read cycle.
  void set_timer() {
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
    current_timer_ = queue_.make_deadline_timer(
        deadline, "session/set_timer/ttl_refresh",
        [this](auto const& op, bool ok) { this->on_timeout(op, ok); });
  }

  /// Handle the timer expiration, Write() a new LeaseKeepAlive request.
  void on_timeout(detail::deadline_timer const& op, bool ok) {
    if (not ok) {
      // ... this is a canceled timer ...
      return;
    }
    {
      std::lock_guard<std::mutex> lock(mu_);
      if (state_ == state::shuttingdown or state_ == state::shutdown) {
        return;
      }
    }
    etcdserverpb::LeaseKeepAliveRequest req;
    req.set_id(lease_id());

    queue_.async_write(
        *ka_stream_, std::move(req), "session/on_timeout/write",
        [this](auto op, bool ok) { this->on_write(op, ok); });
  }

  /// Handle the Write() completion, schedule a new LeaseKeepAlive Read().
  void on_write(ka_stream_type::write_op& op, bool ok) {
    if (not ok) {
      // TODO() - consider logging or exceptions in this case (canceled
      // operation) ...
      return;
    }
    {
      std::lock_guard<std::mutex> lock(mu_);
      if (state_ == state::shuttingdown or state_ == state::shutdown) {
        return;
      }
    }
    queue_.async_read(
        *ka_stream_, "session/on_write/read",
        [this](auto op, bool ok) { this->on_read(op, ok); });
  }

  /// Handle the Read() completion, schedule a new Timer().
  void on_read(ka_stream_type::read_op& op, bool ok) {
    if (not ok) {
      // TODO() - consider logging or exceptions in this case (canceled
      // operation) ...
      return;
    }
    {
      std::lock_guard<std::mutex> lock(mu_);
      if (state_ == state::shuttingdown or state_ == state::shutdown) {
        return;
      }
    }

    // ... the KeepAliveResponse may have a new TTL value, that is the
    // etcd server may be telling us to backoff a little ...
    actual_TTL_ = std::chrono::seconds(op.response.ttl());
    set_timer();
  }

private:
  completion_queue_type& queue_;
};

} // namespace detail
} // namespace etcd
} // namespace jb

#endif // jb_detail_etcd_session_impl_hpp
