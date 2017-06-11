/**
 * @file
 *
 * A program to replay raw ITCH-5.x files as MoldUDP packets.
 *
 * This program replays a ITCH-5.x file via multicast, simulating the
 * behavior of a market data feed.
 */
#include <jb/ehs/acceptor.hpp>
#include <jb/config_object.hpp>
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
  jb::config_attribute<config, jb::log::config> log;
};

class replayer_control {
public:
  replayer_control();

  std::string replay(std::string const& filename);
  void stop_replay(std::string const& token);

  void status(jb::ehs::response_type& res) const;
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
  auto replayer = std::make_shared<replayer_control>();

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
          res.body = std::string(
              "An internal error occurred\r\n"
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
std::string const primary_destination = "::1";
std::string const secondary_destination = "127.0.0.1";
int const primary_port = 12300;
int const secondary_port = 12301;

std::string control_host = "0.0.0.0";
int const control_port = 23000;
} // namespace defaults

config::config()
    : primary_destination(
          desc("primary-destination")
              .help(
                  "The destination for the UDP messages. "
                  "The destination can be a unicast or multicast address."),
          this, defaults::primary_destination)
    , primary_port(
          desc("primary-port")
              .help("The destination port for the UDP messages."),
          this, defaults::primary_port)
    , secondary_destination(
          desc("secondary-destination")
              .help(
                  "The destination for the UDP messages. "
                  "The destination can be empty, a unicast, or a multicast "
                  "address."),
          this, defaults::secondary_destination)
    , secondary_port(
          desc("secondary-port")
              .help("The destination port for the UDP messages."),
          this, defaults::secondary_port)
    , control_host(
          desc("control-host")
              .help(
                  "Where does the server listen for control connections."
                  "Typically this is an address for the current host,"
                  "for example: 'localhost', '0.0.0.0', or '::1'."),
          this, defaults::control_host)
    , control_port(
          desc("control-port").help("The port to receive control connections."),
          this, defaults::control_port)
    , log(desc("log", "logging"), this) {
}

void config::validate() const {
  if (primary_destination() == "") {
    throw jb::usage("Missing primary-destination argument (or setting).", 1);
  }
  log().validate();
}

replayer_control::replayer_control() {
}

void replayer_control::status(jb::ehs::response_type& res) const {
  res.fields.replace("content-type", "text/plain");
  res.body = "Nothing to see here folks\n";
}

} // anonymous namespace
