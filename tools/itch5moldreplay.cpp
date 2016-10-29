#include <jb/itch5/process_iostream_mlist.hpp>
#include <jb/itch5/mold_udp_pacer.hpp>
#include <jb/fileio.hpp>
#include <jb/log.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/tokenizer.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <unordered_map>

namespace {

class config : public jb::config_object {
public:
  config();
  config_object_constructors(config);

  void validate() const override;

  jb::config_attribute<config, std::string> input_file;
  jb::config_attribute<config, std::string> destination;
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
      , pacer_(cfg) {
    socket_.connect(ep);
  }

  /// Handle all messages as blobs
  void handle_unknown(time_point const& recv_ts,
                      jb::itch5::unknown_message const& msg) {
    auto sink = [this](auto buffers) { socket_.send_to(buffers, endpoint_); };
    auto sleeper = [](jb::itch5::mold_udp_pacer<>::duration const& d) {
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

  boost::tokenizer<boost::char_separator<char>> tok(
      cfg.destination(), boost::char_separator<char>(":"));
  std::vector<std::string> tokens(tok.begin(), tok.end());
  if (tokens.size() != 2) {
    throw jb::usage("--destination must be in host:port format.", 1);
  }

  boost::asio::io_service io_service;
  using boost::asio::ip::udp;
  udp::socket s(io_service, udp::endpoint(udp::v4(), 0));
  udp::resolver resolver(io_service);
  udp::endpoint endpoint = *resolver.resolve({udp::v4(), tokens[0], tokens[1]});

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

config::config()
    : input_file(
          desc("input-file").help("An input file with ITCH-5.0 messages."),
          this)
    , destination(
          desc("destination")
              .help("The destination for the UDP messages, in address:port "
                    "format. "
                    "The destination can be a unicast or multicast address."),
          this)
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
}

} // anonymous namespace
