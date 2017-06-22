#include <jb/config_object.hpp>
#include <jb/log.hpp>

#include <boost/asio.hpp>
#include <etcd/etcdserver/etcdserverpb/rpc.grpc.pb.h>
#include <grpc++/alarm.h>
#include <grpc++/grpc++.h>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <thread>

/**
 * Define types and functions used in this program.
 */
namespace {
/// Configuration parameters for moldfeedhandler
class config : public jb::config_object {
public:
  config();
  config_object_constructors(config);

  void validate() const override;

  jb::config_attribute<config, std::string> etcd_address;
};

class session {
public:
  /// How many KeepAlive requests we send per TTL cyle.
  // TODO() - the magic number 5  should be a configurable parameter.
  static int constexpr keep_alives_per_ttl = 5;

  template <typename duration_type>
  session(
      std::shared_ptr<grpc::Channel> etcd_service, duration_type desired_TTL)
      : session(
            etcd_service,
            std::chrono::duration_cast<std::chrono::milliseconds>(desired_TTL),
            true) {
  }

  /// Shutdown the
  void shutdown() {
    // TODO() - the shutdown is more complicated, we first need to
    // revoke the cancel the timers (if any), revoke the lease, or at
    // least try to.  then finish the reader-writer stream, then
    // shutdown the queue ...
    queue_.Shutdown();
  }

  void run() try {
    // This function is wrapped in try/catch block because it runs in
    // its own thread.  If we let exceptions scape the program would
    // terminate.
    // TODO() - maybe we should let the program terminate in that
    // case, the application would lose the master election.  I
    // believe that should be a matter of policy, so for the moment
    // log ...
    void* tag = nullptr;
    bool ok = false;
    while (queue_.Next(&tag, &ok) and ok) {
      if (tag == nullptr) {
        continue;
      }
      // ... tag must be a pointer to std::function<void()> see the
      // comments for tag_set_timer_ and friends to understand why ...
      auto callback = static_cast<std::function<void()>*>(tag);
      (*callback)();
    }
  } catch (std::exception const& ex) {
    JB_LOG(info) << "Standard C++ exception in session::run(): " << ex.what();
  } catch (...) {
    JB_LOG(info) << "Unknown exception in session::run()";
  }

private:
  /**
   * Constructor
   *
   * TODO() - we need to add some dependency injection for unit
   * testing ...
   */
  session(
      std::shared_ptr<grpc::Channel> etcd_service,
      std::chrono::milliseconds desired_TTL, bool)
      : etcd_service_(etcd_service)
      , desired_TTL_(desired_TTL)
      , actual_TTL_(desired_TTL)
      , queue_()
      , stub_(etcdserverpb::Lease::NewStub(etcd_service_))
      , lease_id_(0)
      , kactx_()
      , rdwr_()
      , alarm_()
      , ka_req_()
      , ka_res_()
      , tag_set_timer_(make_tag(&session::set_timer))
      , tag_on_timeout_(make_tag(&session::on_timeout))
      , tag_on_write_(make_tag(&session::on_write))
      , tag_on_read_(make_tag(&session::on_read)) {
    start();
  }

  void start() {
    // ... request a new lease from etcd(1) ...
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
                 << "    header.cluster_id=" << resp.header().cluster_id()
                 << "\n"
                 << "    header.member_id=" << resp.header().member_id() << "\n"
                 << "    header.revision=" << resp.header().revision() << "\n"
                 << "    header.raft_term=" << resp.header().raft_term() << "\n"
                 << "  ID=" << std::hex << std::setw(16) << std::setfill('0')
                 << resp.id() << "  TTL=" << std::dec << resp.ttl() << "s\n";
    lease_id_ = resp.id();
    actual_TTL_ = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::seconds(resp.ttl()));

    // ... create the reader-writer to send KeepAlive requests and
    // receive the corresponding responses, call set_timer() when done
    // the reader/writer is ready ...
    rdwr_ = stub_->AsyncLeaseKeepAlive(&kactx_, &queue_, &tag_set_timer_);
  }

  /// Set a timer to start the next Write/Read cycle.
  void set_timer() {
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
  void on_timeout() {
    ka_req_.set_id(lease_id_);
    rdwr_->Write(ka_req_, static_cast<void*>(&tag_on_write_));
  }

  /// Handle the Write() completion, schedule a new Read().
  void on_write() {
    rdwr_->Read(&ka_res_, static_cast<void*>(&tag_on_read_));
  }

  /// Handle the Read() completion, schedule a new Timer().
  void on_read() {
    // ... the KeepAliveResponse may have a new TTL value, that is the
    // etcd server may be telling us to backoff a little ...
    actual_TTL_ = std::chrono::seconds(ka_res_.ttl());
    // TODO() - remove debugging log ...
    JB_LOG(debug) << "reset timer to " << actual_TTL_.count() << "ms"
                  << ", received=" << ka_res_.ttl() << "s"
                  << ", desired=" << desired_TTL_.count() << "ms";
    set_timer();
  }

  typedef void (session::*callback_member)();
  std::function<void()> make_tag(callback_member member) {
    return std::function<void()>([this, member]() { (this->*member)(); });
  }

private:
  std::shared_ptr<grpc::Channel> etcd_service_;
  std::chrono::milliseconds desired_TTL_;
  std::chrono::milliseconds actual_TTL_;
  // TODO() - I do not think the queue belongs in this object, seems
  // like it should be a parameter, if nothing else for dependency
  // injection ...
  grpc::CompletionQueue queue_;
  std::unique_ptr<etcdserverpb::Lease::Stub> stub_;
  std::uint64_t lease_id_;

  /// The context for the asynchronous KeepAlive request.
  grpc::ClientContext kactx_;
  using ReaderWriter = grpc::ClientAsyncReaderWriter<
      etcdserverpb::LeaseKeepAliveRequest,
      etcdserverpb::LeaseKeepAliveResponse>;
  std::unique_ptr<ReaderWriter> rdwr_;

  //@{
  /**
   * Hold data for asynchronous operations.
   */
  std::unique_ptr<grpc::Alarm> alarm_;
  etcdserverpb::LeaseKeepAliveRequest ka_req_;
  etcdserverpb::LeaseKeepAliveResponse ka_res_;
  //@}

  //@{
  /**
   * Tags / callback for asynchronous operations.
   *
   * gRPC++ tags are awkward, they should have used realcallbacks, ala
   * Boost.ASIO.  We need something that is convertible to and from a
   * void*.  We could create an object for each asynchronous
   * operation, but that is slow, and also extra weird.  Instead we
   * create a number of member objects, one for each asynchronous
   * operation.  Each member object is a std::function<void()>, which
   * we can call from the completion queue loop.  The other
   * alternatives I though about do not work:
   *   - Pointer to member functions are not covertible to void*.
   *   - A pointer to a lambda would not work, because we need to
   *     capture at least the [this] pointer, and pointer to
   *     lambdas with captures are *not* pointer to functions.
   *   - Also the ponter to lambda would need to be stored
   *     somewhere.
   */
  std::function<void()> tag_set_timer_;
  std::function<void()> tag_on_timeout_;
  std::function<void()> tag_on_write_;
  std::function<void()> tag_on_read_;
  //@}
};

int constexpr session::keep_alives_per_ttl;
} // anonymous namespace

int main(int argc, char* argv[]) try {
  // Do the usual thin in JayBeams to load the configuration ...
  config cfg;
  cfg.load_overrides(
      argc, argv, std::string("election_listener.yaml"), "JB_ROOT");

  // TODO() - use the default credentials when possible, should be
  // controlled by a configuration parameter ...
  std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(
      cfg.etcd_address(), grpc::InsecureChannelCredentials());

  // ... a session is the JayBeams abstraction to hold a etcd lease ...
  // TODO() - make the initial TTL configurable.
  // TODO() - decide if storing the TTL in milliseconds makes any
  // sense, after all etcd uses seconds ...
  session sess(channel, std::chrono::seconds(10));

  // ... to run multiple things asynchronously in gRPC++ we need a
  // completion queue.  Unfortunately cannot share this with
  // Boost.ASIO queue.  Which is a shame, so run a separate thread...
  // TODO() - the thread should be configurable, and use
  // jb::thread_launcher.
  std::thread t([&sess]() { sess.run(); });

  // ... use Boost.ASIO to wait for a signal, then terminate the
  // session ...
  boost::asio::io_service io;

  // TODO() - do the signal thing
  std::this_thread::sleep_for(std::chrono::minutes(1));

  sess.shutdown();

  t.join();

  return 0;
} catch (jb::usage const& u) {
  std::cerr << u.what() << std::endl;
  return u.exit_status();
} catch (std::exception const& ex) {
  std::cerr << "Standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "Unknown exception raised" << std::endl;
  return 1;
}

namespace {
namespace defaults {
std::string const etcd_address = "localhost:2379";
} // namespace defaults

config::config()
    : etcd_address(
          desc("etcd-address").help("The address for the etcd server."), this,
          defaults::etcd_address) {
}

void config::validate() const {
}
} // anonymous namespace