#include <jb/itch5/process_iostream_mlist.hpp>
#include <jb/fileio.hpp>
#include <jb/log.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <unordered_map>

namespace {

class config : public jb::config_object {
 public:
  config();
  config_object_constructors(config);

  void validate() const override;

  jb::config_attribute<config,std::string> input_file;
  jb::config_attribute<config,std::string> destination;
  jb::config_attribute<config,jb::log::config> log;
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
  replayer() {}

  /// Handle all messages as blobs
  void handle_unknown(
      time_point const& recv_ts, long msgcnt, std::size_t msgoffset,
      char const* msgbuf, std::size_t msglen) {}

  /// Return the current timestamp for delay measurements
  time_point now() const {
    return std::chrono::steady_clock::now();
  }
};

} // anonymous namespace

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.load_overrides(
      argc, argv, std::string("itch5moldreplay.yaml"), "JB_ROOT");
  jb::log::init(cfg.log());

  boost::iostreams::filtering_istream in;
  jb::open_input_file(in, cfg.input_file());

  replayer replay;
  jb::itch5::process_iostream_mlist<replayer>(in, replay);

  return 0;
} catch(jb::usage const& u) {
  std::cerr << u.what() << std::endl;
  return u.exit_status();
} catch(std::exception const& ex) {
  std::cerr << "Standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch(...) {
  std::cerr << "Unknown exception raised" << std::endl;
  return 1;
}

namespace {

config::config()
    : input_file(desc("input-file").help(
        "An input file with ITCH-5.0 messages."), this)
    , destination(desc("destination").help(
        "The destination for the UDP messages, in address:port format. "
        "The destination can be a unicast or multicast address."), this)
    , log(desc("log", "logging"), this)
{}

void config::validate() const {
  if (input_file() == "") {
    throw jb::usage(
        "Missing input-file setting."
        "  You must specify an input file.", 1);
  }
  log().validate();
}

} // anonymous namespace
