#include <jb/itch5/process_iostream_mlist.hpp>
#include <jb/itch5/mold_udp_pacer.hpp>
#include <jb/fileio.hpp>
#include <jb/log.hpp>
#include <jb/as_hhmmss.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/ip/multicast.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {

class config : public jb::config_object {
public:
  config();
  config_object_constructors(config);

  void validate() const override;

  jb::config_attribute<config, std::string> input_file;
  jb::config_attribute<config, std::string> destination;
  jb::config_attribute<config, int> port;
  jb::config_attribute<config, jb::log::config> log;
  jb::config_attribute<config, jb::itch5::mold_udp_pacer_config> pacer;
};

class replayer {
public:
  //@{
  /**
   * @name Type traits
   */
  typedef std::chrono::steady_clock::time_point time_point;
  //@}

  /// Initialize an empty handler
  replayer(boost::asio::ip::udp::socket&& s,
           boost::asio::ip::udp::endpoint const& ep,
           jb::itch5::mold_udp_pacer_config const& cfg)
      : socket_(std::move(s))
      , endpoint_(ep)
      , pacer_(cfg, jb::itch5::mold_udp_pacer<>::session_id_type("ITCH/RPLY")) {
  }

  /// Handle all messages as blobs
  void handle_unknown(time_point const& recv_ts,
                      jb::itch5::unknown_message const& msg) {
    auto sink = [this](auto buffers) { socket_.send_to(buffers, endpoint_); };
    auto sleeper = [](jb::itch5::mold_udp_pacer<>::duration d) {
      if (d > std::chrono::seconds(10)) {
        JB_LOG(info) << "Sleep request for " << jb::as_hh_mm_ss_u(d);
        d = std::chrono::seconds(10);
      }
      std::this_thread::sleep_for(d);
    };
    pacer_.handle_message(recv_ts, msg, sink, sleeper);
  }

  /// Return the current timestamp for delay measurements
  time_point now() const {
    return std::chrono::steady_clock::now();
  }

private:
  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint endpoint_;
  jb::itch5::mold_udp_pacer<> pacer_;
};

} // anonymous namespace

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.load_overrides(argc, argv, std::string("itch5moldreplay.yaml"),
                     "JB_ROOT");
  jb::log::init(cfg.log());

  boost::asio::io_service io_service;
  auto address = boost::asio::ip::address::from_string(cfg.destination());
  boost::asio::ip::udp::endpoint endpoint(address, cfg.port());
  JB_LOG(info) << "Sending to endpoint=" << endpoint;
  boost::asio::ip::udp::socket s(io_service, endpoint.protocol());
  s.set_option(boost::asio::ip::multicast::enable_loopback(true));

  boost::iostreams::filtering_istream in;
  jb::open_input_file(in, cfg.input_file());

  replayer rep(std::move(s), endpoint, cfg.pacer());
  jb::itch5::process_iostream_mlist<replayer>(in, rep);

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

int default_multicast_port() {
  return 50000;
}

std::string default_multicast_group() {
  return "::1";
}

config::config()
    : input_file(
          desc("input-file").help("An input file with ITCH-5.0 messages."),
          this)
    , destination(
          desc("destination")
              .help("The destination for the UDP messages. "
                    "The destination can be a unicast or multicast address."),
          this, default_multicast_group())
    , port(desc("port").help("The destination port for the UDP messages. "),
           this, default_multicast_port())
    , log(desc("log", "logging"), this)
    , pacer(desc("pacer", "mold-udp-pacer"), this) {
}

void config::validate() const {
  if (input_file() == "") {
    throw jb::usage("Missing input-file setting."
                    "  You must specify an input file.",
                    1);
  }
  log().validate();
  pacer().validate();
}

} // anonymous namespace
