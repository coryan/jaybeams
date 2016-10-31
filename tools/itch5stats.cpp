#include <jb/itch5/process_iostream.hpp>
#include <jb/fileio.hpp>
#include <jb/log.hpp>
#include <jb/offline_feed_statistics.hpp>

#include <iostream>
#include <stdexcept>

namespace {

class config : public jb::config_object {
public:
  config();
  config_object_constructors(config);

  void validate() const override;

  jb::config_attribute<config, std::string> input_file;
  jb::config_attribute<config, jb::offline_feed_statistics::config> stats;
};

/**
 * An implementation of jb::message_handler_concept to capture
 * ITCH-5.0 statistics.
 */
class itch5_stats_handler {
public:
  itch5_stats_handler(config const& cfg)
      : stats_(cfg.stats()) {
  }

  typedef std::chrono::steady_clock::time_point time_point;

  time_point now() const {
    return std::chrono::steady_clock::now();
  }

  template <typename message_type>
  void handle_message(
      time_point recv_ts, long msgcnt, std::size_t msgoffset,
      message_type const& msg) {
    JB_LOG(trace) << msgcnt << ":" << msgoffset << " " << msg;
    auto pl = now() - recv_ts;
    stats_.sample(msg.header.timestamp.ts, pl);
  }

  void
  handle_unknown(time_point recv_ts, jb::itch5::unknown_message const& msg) {
    char msgtype = *static_cast<char const*>(msg.buf());
    JB_LOG(error) << "Unknown message type '" << msgtype << "'(" << int(msgtype)
                  << ") in msgcnt=" << msg.count()
                  << ", msgoffset=" << msg.offset();
  }

private:
  jb::offline_feed_statistics stats_;
};

} // anonymous namespace

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.load_overrides(argc, argv, std::string("itch5stats.yaml"), "JB_ROOT");
  jb::log::init();

  boost::iostreams::filtering_istream in;
  jb::open_input_file(in, cfg.input_file());

  itch5_stats_handler handler(cfg);
  jb::itch5::process_iostream(in, handler);

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
config::config()
    : input_file(
          desc("input-file").help("An input file with ITCH-5.0 messages."),
          this)
    , stats(desc("stats", "offline-feed-statistics"), this) {
}

void config::validate() const {
  if (input_file() == "") {
    throw jb::usage(
        "Missing input-file setting."
        "  You must specify an input file.",
        1);
  }
  stats().validate();
}

} // anonymous namespace
