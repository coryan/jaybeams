/**
 * @file
 *
 * A program to replay raw ITCH-5.x files as MoldUDP packets.
 *
 * This program replays a ITCH-5.x file via multicast, simulating the
 * behavior of a market data feed.
 */
#include <jb/ehs/request_dispatcher.hpp>
#include <jb/config_object.hpp>
#include <jb/log.hpp>

#include <beast/http.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/multicast.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/strand.hpp>

#include <atomic>
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

using endpoint_type = boost::asio::ip::tcp::endpoint;
using address_type = boost::asio::ip::address;
using socket_type = boost::asio::ip::tcp::socket;

using jb::ehs::request_dispatcher;
using jb::ehs::request_type;
using jb::ehs::response_type;

/**
 * Handle one connection to the control server.
 */
class connection : public std::enable_shared_from_this<connection> {
public:
  /// Constructor
  explicit connection(
      socket_type&& sock, std::shared_ptr<request_dispatcher> dispatcher);

  /// Asynchronously read a HTTP request for this connection.
  void run();

  //@{
  /**
   * Control copy and assignment.
   */
  connection(connection&&) = default;
  connection(connection const&) = default;
  connection& operator=(connection&&) = delete;
  connection& operator=(connection const&) = delete;
  //@}

private:
  /**
   * Handle a completed HTTP request read.
   *
   * Once a HTTP request has been received it needs to be parsed and a
   * response sent back.  This function is called by the Beast
   * framework when the read completes.
   *
   * @param ec the error code
   */
  void on_read(boost::system::error_code const& ec);

  /**
   * Handle a completed response write.
   *
   * @param ec indicate if writing the response resulted in an error.
   */
  void on_write(boost::system::error_code const& ec);

private:
  socket_type sock_;
  std::shared_ptr<request_dispatcher> dispatcher_;
  boost::asio::io_service::strand strand_;
  int id_;
  beast::streambuf sb_;
  request_type req_;

  static std::atomic<int> idgen;
};

/**
 * Create a control server for the program.
 *
 * The program runs as a typical daemon, accepting HTTP requests to
 * start new replays, stop them, and show its current status.
 */
class control_server {
public:
  /**
   * Create a new control server
   *
   * @param io Boost.ASIO service used to demux I/O events for this
   * control server.
   * @param ep the endpoint this control server listens on.
   * @param dispatcher the object to process requests
   */
  control_server(
      boost::asio::io_service& io, endpoint_type const& ep,
      std::shared_ptr<request_dispatcher> dispatcher);

private:
  /**
   * Handle a completed asynchronous accept() call.
   */
  void on_accept(boost::system::error_code const& ec);

private:
  boost::asio::ip::tcp::acceptor acceptor_;
  std::shared_ptr<request_dispatcher> dispatcher_;
  // Hold the results of an  asynchronous accept() operation.
  socket_type sock_;
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

  auto dispatcher = std::make_shared<request_dispatcher>("moldreplay");
  dispatcher->add_handler("/", [](request_type const&, response_type& res) {
    res.fields.insert("Content-type", "text/plain");
    res.body = "Server running...\r\n";
  });
  dispatcher->add_handler(
      "/config", [&cfg](request_type const&, response_type& res) {
        res.fields.insert("Content-type", "text/plain");
        std::ostringstream os;
        os << cfg << "\r\n";
        res.body = os.str();
      });
  control_server server(io_service, ep, dispatcher);

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
std::string const secondary_destination = "";
int const primary_port = 50000;
int const secondary_port = 50001;

std::string control_host = "0.0.0.0";
int const control_port = 40000;
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

connection::connection(
    socket_type&& sock, std::shared_ptr<request_dispatcher> dispatcher)
    : sock_(std::move(sock))
    , dispatcher_(dispatcher)
    , strand_(sock_.get_io_service())
    , id_(++idgen) {
}

void connection::run() {
  beast::http::async_read(
      sock_, sb_, req_,
      strand_.wrap(
          [self = shared_from_this()](boost::system::error_code const& ec) {
            self->on_read(ec);
          }));
}

void connection::on_read(boost::system::error_code const& ec) {
  if (ec) {
    JB_LOG(info) << "#" << id_ << " on_read: " << ec.message();
    return;
  }
  // Prepare a response ...
  response_type res = dispatcher_->process(req_);
  beast::http::prepare(res);
  // ... and send it back ...
  beast::http::async_write(
      sock_, std::move(res),
      [self = shared_from_this()](boost::system::error_code const& ec) {
        self->on_write(ec);
      });
}

void connection::on_write(boost::system::error_code const& ec) {
  if (ec) {
    JB_LOG(info) << "#" << id_ << " on_read: " << ec.message();
    return;
  }
  // Start a new asynchronous read for the next message ...
  run();
}

// Define the generator for connection ids ...
std::atomic<int> connection::idgen(0);

control_server::control_server(
    boost::asio::io_service& io, endpoint_type const& ep,
    std::shared_ptr<request_dispatcher> dispatcher)
    : acceptor_(io)
    , dispatcher_(dispatcher)
    , sock_(io) {
  acceptor_.open(ep.protocol());
  acceptor_.bind(ep);
  acceptor_.listen(boost::asio::socket_base::max_connections);
  // ... schedule an asynchronous accept() operation into the new
  // socket ...
  acceptor_.async_accept(sock_, [this](boost::system::error_code const& ec) {
    this->on_accept(ec);
  });
}

void control_server::on_accept(boost::system::error_code const& ec) {
  // Return when the acceptor is closed or there is an error ...
  if (not acceptor_.is_open()) {
    return;
  }
  if (ec) {
    JB_LOG(info) << "accept: " << ec.message();
    return;
  }
  // ... move the newly created socket to a stack variable so we can
  // schedule a new asynchronous accept ...
  socket_type sock(std::move(sock_));
  acceptor_.async_accept(sock_, [this](boost::system::error_code const& ec) {
    this->on_accept(ec);
  });
  // ... create a new connection for the newly created socket and
  // schedule it ...
  auto c = std::make_shared<connection>(std::move(sock), dispatcher_);
  c->run();
}

} // anonymous namespace
