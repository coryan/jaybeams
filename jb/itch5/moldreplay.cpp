/**
 * @file
 *
 * A program to replay raw ITCH-5.x files as MoldUDP packets.
 *
 * This program replays a ITCH-5.x file via multicast, simulating the
 * behavior of a market data feed.
 */
#include <jb/ehs/acceptor.hpp>
#include <jb/itch5/mold_udp_pacer.hpp>
#include <jb/itch5/process_iostream_mlist.hpp>
#include <jb/as_hhmmss.hpp>
#include <jb/config_object.hpp>
#include <jb/fileio.hpp>
#include <jb/launch_thread.hpp>
#include <jb/log.hpp>

#include <beast/http.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/multicast.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

/// Types and functions used in this program
namespace {
/**
 * Program configuration.
 */
class config : public jb::config_object {
public:
  config();
  config_object_constructors(config);

  void validate() const override;

  jb::config_attribute<config, std::string> primary_destination;
  jb::config_attribute<config, int> primary_port;
  jb::config_attribute<config, std::string> secondary_destination;
  jb::config_attribute<config, int> secondary_port;
  jb::config_attribute<config, std::string> control_host;
  jb::config_attribute<config, unsigned short> control_port;
  jb::config_attribute<config, std::string> input_file;
  jb::config_attribute<config, jb::thread_config> replay_session;
  jb::config_attribute<config, jb::itch5::mold_udp_pacer_config> pacer;
  jb::config_attribute<config, jb::log::config> log;
};

class session : public std::enable_shared_from_this<session> {
public:
  //@{
  /**
   * @name Type traits
   */
  typedef std::chrono::steady_clock::time_point time_point;
  //@}

  /// Create a new session to replay a ITCH-5.x file.
  session(config const& cfg);

  /// Start running a new session
  void start();

  /// Stop a running session
  void stop();

  /// The last message count processed
  std::uint32_t last_message_count() const {
    return last_message_count_.load(std::memory_order_relaxed);
  }

  /// The offset of the last message processed
  std::uint64_t last_message_offset() const {
    return last_message_offset_.load(std::memory_order_relaxed);
  }

  /// Implement the callback for jb::itch5::process_iostream_mlist<>
  void handle_unknown(
      time_point const& recv_ts, jb::itch5::unknown_message const& msg);

  /// Return the current timestamp for delay measurements
  time_point now() const {
    return std::chrono::steady_clock::now();
  }

private:
  config cfg_;
  std::atomic_bool stop_;
  jb::itch5::mold_udp_pacer<> pacer_;
  std::atomic<std::uint32_t> last_message_count_;
  std::atomic<std::uint64_t> last_message_offset_;
  boost::asio::io_service io_;
  boost::asio::ip::udp::socket s0_;
  boost::asio::ip::udp::endpoint ep0_;
  boost::asio::ip::udp::socket s1_;
  boost::asio::ip::udp::endpoint ep1_;
  bool ep1_enabled_;
};

class replayer_control {
public:
  explicit replayer_control(config const& cfg);

  enum class state { idle, starting, replaying, stopping };

  void status(jb::ehs::response_type& res) const;
  void start(jb::ehs::request_type const& req, jb::ehs::response_type& res);
  void stop(jb::ehs::request_type const& req, jb::ehs::response_type& res);

private:
  bool start_check();
  void replay_done();

private:
  config cfg_;
  mutable std::mutex mu_;
  state current_state_;
  std::thread session_thread_;
  std::shared_ptr<session> session_;
};

} // anonymous namespace

int main(int argc, char* argv[]) try {
  // Load the configuration ...
  config cfg;
  cfg.load_overrides(argc, argv, std::string("moldreplay.yaml"), "JB_ROOT");
  jb::log::init(cfg.log());

  boost::asio::io_service io_service;

  using endpoint = boost::asio::ip::tcp::endpoint;
  using address = boost::asio::ip::address;
  endpoint ep{address::from_string(cfg.control_host()), cfg.control_port()};

  // ... create the replayer control, this is where the main work
  // happens ...
  auto replayer = std::make_shared<replayer_control>(cfg);

  // ... create a dispatcher to process the HTTP requests, register
  // some basic handlers ...
  using jb::ehs::request_type;
  using jb::ehs::response_type;
  auto dispatcher = std::make_shared<jb::ehs::request_dispatcher>("moldreplay");
  dispatcher->add_handler("/", [](request_type const&, response_type& res) {
    res.fields.insert("Content-type", "text/plain");
    res.body = "Server running...\r\n";
  });
  dispatcher->add_handler(
      "/config", [cfg](request_type const&, response_type& res) {
        res.fields.insert("Content-type", "text/plain");
        std::ostringstream os;
        os << cfg << "\r\n";
        res.body = os.str();
      });
  // ... we need to use a weak_ptr to avoid a cycle of shared_ptr ...
  std::weak_ptr<jb::ehs::request_dispatcher> disp = dispatcher;
  dispatcher->add_handler(
      "/metrics", [disp](request_type const&, response_type& res) {
        std::shared_ptr<jb::ehs::request_dispatcher> d(disp);
        if (not d) {
          res.status = 500;
          res.reason = beast::http::reason_string(res.status);
          res.body = std::string("An internal error occurred\r\n"
                                 "Null request handler in /metrics\r\n");
          return;
        }
        res.fields.replace("content-type", "text/plain; version=0.0.4");
        d->append_metrics(res);
      });
  dispatcher->add_handler(
      "/replay-status", [replayer](request_type const&, response_type& res) {
        replayer->status(res);
      });
  dispatcher->add_handler(
      "/replay-start", [replayer](request_type const& req, response_type& res) {
        replayer->start(req, res);
      });
  dispatcher->add_handler(
      "/replay-stop", [replayer](request_type const& req, response_type& res) {
        replayer->stop(req, res);
      });

  // ... create an acceptor to handle incoming connections, if we wanted
  // to, we could create multiple acceptors on different addresses
  // pointing to the same dispatcher ...
  jb::ehs::acceptor acceptor(io_service, ep, dispatcher);

  // ... run the program forever ...
  io_service.run();

  return 0;
} catch (jb::usage const& u) {
  std::cout << u.what() << std::endl;
  return u.exit_status();
} catch (std::exception const& ex) {
  std::cerr << "Standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "Unknown exception raised" << std::endl;
  return 1;
}

namespace {

/// Default values for the program configuration
namespace defaults {
std::string const primary_destination = "127.0.0.1";
std::string const secondary_destination = "127.0.0.1";
int const primary_port = 12300;
int const secondary_port = 12301;

std::string control_host = "0.0.0.0";
int const control_port = 23000;
} // namespace defaults

config::config()
    : primary_destination(
          desc("primary-destination")
              .help("The destination for the UDP messages. "
                    "The destination can be a unicast or multicast address."),
          this, defaults::primary_destination)
    , primary_port(
          desc("primary-port")
              .help("The destination port for the UDP messages."),
          this, defaults::primary_port)
    , secondary_destination(
          desc("secondary-destination")
              .help("The destination for the UDP messages. "
                    "The destination can be empty, a unicast, or a multicast "
                    "address."),
          this, defaults::secondary_destination)
    , secondary_port(
          desc("secondary-port")
              .help("The destination port for the UDP messages."),
          this, defaults::secondary_port)
    , control_host(
          desc("control-host")
              .help("Where does the server listen for control connections."
                    "Typically this is an address for the current host,"
                    "for example: 'localhost', '0.0.0.0', or '::1'."),
          this, defaults::control_host)
    , control_port(
          desc("control-port").help("The port to receive control connections."),
          this, defaults::control_port)
    , input_file(
          desc("input-file").help("The file to replay when requested."), this)
    , replay_session(
          desc("replay-session", "thread-config")
              .help("Configure the replay session threads."),
          this, jb::thread_config().name("replay"))
    , pacer(
          desc("pacer", "mold-udp-pacer").help("Configure the ITCH-5.x pacer"),
          this)
    , log(desc("log", "logging"), this) {
}

void config::validate() const {
  if (primary_destination() == "") {
    throw jb::usage("Missing primary-destination argument or setting.", 1);
  }
  if (input_file() == "") {
    throw jb::usage("Missing input-file argument or setting.", 1);
  }
  log().validate();
}

session::session(config const& cfg)
    : cfg_(cfg)
    , stop_(false)
    , pacer_(cfg.pacer())
    , last_message_count_(0)
    , last_message_offset_(0)
    , io_()
    , s0_(io_)
    , ep0_()
    , s1_(io_)
    , ep1_()
    , ep1_enabled_(false) {
#ifndef ATOMIC_BOOL_LOCK_FREE
#error "Missing ATOMIC_BOOL_LOCK_FREE required by C++11 standard"
#endif // ATOMIC_BOOL_LOCK_FREE
  static_assert(
      ATOMIC_BOOL_LOCK_FREE == 2, "Class requires lock-free std::atomic<bool>");

  auto address0 =
      boost::asio::ip::address::from_string(cfg_.primary_destination());
  boost::asio::ip::udp::endpoint ep0(address0, cfg_.primary_port());
  boost::asio::ip::udp::socket s0(io_, ep0.protocol());
  if (ep0.address().is_multicast()) {
    s0.set_option(boost::asio::ip::multicast::enable_loopback(true));
  }
  s0_ = std::move(s0);
  ep0_ = std::move(ep0);

  if (cfg_.secondary_destination() != "") {
    auto address1 =
        boost::asio::ip::address::from_string(cfg_.secondary_destination());
    boost::asio::ip::udp::endpoint ep1(address1, cfg_.secondary_port());
    boost::asio::ip::udp::socket s1(io_, ep1.protocol());
    if (ep1.address().is_multicast()) {
      s1.set_option(boost::asio::ip::multicast::enable_loopback(true));
    }
    s1_ = std::move(s1);
    ep1_ = std::move(ep1);
    ep1_enabled_ = true;
  }
}

void session::start() {
  auto self = shared_from_this();

  boost::iostreams::filtering_istream in;
  jb::open_input_file(in, cfg_.input_file());
  jb::itch5::process_iostream_mlist<session>(in, *self);
}

void session::stop() {
  stop_.store(true, std::memory_order_release);
}

void session::handle_unknown(
    time_point const& recv_ts, jb::itch5::unknown_message const& msg) {
  if (stop_.load(std::memory_order_consume)) {
    throw std::runtime_error("stopping replay thread");
  }
  last_message_count_.store(msg.count(), std::memory_order_relaxed);
  last_message_offset_.store(msg.offset(), std::memory_order_relaxed);
  auto sink = [this](auto buffers) {
    s0_.send_to(buffers, ep0_);
    if (ep1_enabled_) {
      s1_.send_to(buffers, ep1_);
    }
  };
  auto sleeper = [](jb::itch5::mold_udp_pacer<>::duration d) {
    // ... never sleep for more than 10 seconds, the feeds typically
    // have large idle times early and waiting for hours to start
    // doing anything interesting is kind of boring ...
    if (d > std::chrono::seconds(10)) {
      d = std::chrono::seconds(10);
    }
    std::this_thread::sleep_for(d);
  };
  pacer_.handle_message(recv_ts, msg, sink, sleeper);
}

replayer_control::replayer_control(config const& cfg)
    : cfg_(cfg)
    , mu_()
    , current_state_(state::idle) {
}

void replayer_control::status(jb::ehs::response_type& res) const {
  res.fields.replace("content-type", "text/plain");

  std::lock_guard<std::mutex> guard(mu_);
  std::ostringstream os;
  switch (current_state_) {
  case state::idle:
    res.body = "idle\nNothing to see here folks\n";
    return;
  case state::starting:
    os << "starting\nMessages arriving shortly\n";
    break;
  case state::stopping:
    os << "stopping\nMessages will stop flowing\n";
    break;
  case state::replaying:
    os << "replaying\n";
    break;
  default:
    res.status = 500;
    res.reason = beast::http::reason_string(res.status);
    res.body = "Unkown state\n";
    return;
  }
  JB_ASSERT_THROW(session_.get() != 0);
  os << "  last-count: " << session_->last_message_count() << "\n"
     << "  last-offset: " << session_->last_message_offset() << "\n"
     << "\n";
  res.body = os.str();
}

void replayer_control::start(
    jb::ehs::request_type const&, jb::ehs::response_type& res) {
  std::lock_guard<std::mutex> guard(mu_);
  if (current_state_ != state::idle) {
    res.status = 400;
    res.reason = beast::http::reason_string(res.status);
    res.body = "request rejected, current status is not idle\n";
    return;
  }
  // ... set the result before any computation, if there is a failure
  // it will raise an exception and the caller sends back the error
  // ...
  res.status = 200;
  res.body = "request succeeded, started new session\n";
  auto s = std::make_shared<session>(cfg_);
  // ... wait until this point to set the state to starting, if there
  // are failures before we have not changed the state and can
  // continue ...
  current_state_ = state::starting;
  jb::launch_thread(session_thread_, cfg_.replay_session(), [s, this]() {
    // ... check if the session can start, maybe it was stopped
    // before the thread started ...
    if (not start_check()) {
      return;
    }
    // ... run the session, without holding the mu_ lock ...
    try {
      s->start();
    } catch (...) {
    }
    // ... reset the state to idle, even if an exception is raised
    // ...
    replay_done();
  });
  session_thread_.detach();
  session_ = s;
}

void replayer_control::stop(
    jb::ehs::request_type const&, jb::ehs::response_type& res) {
  std::lock_guard<std::mutex> guard(mu_);
  if (current_state_ != state::replaying and
      current_state_ != state::starting) {
    res.status = 400;
    res.body = "request rejected, current status is not valid\n";
    return;
  }
  current_state_ = state::stopping;
  res.status = 200;
  res.body = "request succeeded, stopping current session\n";
  JB_ASSERT_THROW(session_.get() != 0);
  session_->stop();
}

bool replayer_control::start_check() {
  std::lock_guard<std::mutex> guard(mu_);
  if (current_state_ != state::starting) {
    return false;
  }
  current_state_ = state::replaying;
  return true;
}

void replayer_control::replay_done() {
  std::lock_guard<std::mutex> guard(mu_);
  current_state_ = state::idle;
  session_.reset();
}

} // anonymous namespace
