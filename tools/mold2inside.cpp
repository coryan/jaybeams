#include <jb/itch5/compute_inside.hpp>
#include <jb/itch5/process_iostream.hpp>
#include <jb/itch5/mold_udp_channel.hpp>
#include <jb/offline_feed_statistics.hpp>
#include <jb/fileio.hpp>
#include <jb/log.hpp>

#include <iostream>
#include <stdexcept>
#include <unordered_map>

namespace {

class config : public jb::config_object {
public:
  config();
  config_object_constructors(config);

  void validate() const override;

  jb::config_attribute<config, int> multicast_port;
  jb::config_attribute<config, std::string> listen_address;
  jb::config_attribute<config, std::string> multicast_group;
  jb::config_attribute<config, std::string> output_file;
  jb::config_attribute<config, jb::log::config> log;
  jb::config_attribute<config, jb::offline_feed_statistics::config> stats;
  jb::config_attribute<config, jb::offline_feed_statistics::config>
      symbol_stats;
  jb::config_attribute<config, bool> enable_symbol_stats;
};

} // anonymous namespace

#define KNOWN_ITCH5_MESSAGES                                                   \
  jb::itch5::add_order_message, jb::itch5::add_order_mpid_message,             \
      jb::itch5::broken_trade_message, jb::itch5::cross_trade_message,         \
      jb::itch5::ipo_quoting_period_update_message,                            \
      jb::itch5::market_participant_position_message,                          \
      jb::itch5::mwcb_breach_message, jb::itch5::mwcb_decline_level_message,   \
      jb::itch5::net_order_imbalance_indicator_message,                        \
      jb::itch5::order_cancel_message, jb::itch5::order_delete_message,        \
      jb::itch5::order_executed_message,                                       \
      jb::itch5::order_executed_price_message,                                 \
      jb::itch5::order_replace_message,                                        \
      jb::itch5::reg_sho_restriction_message,                                  \
      jb::itch5::stock_directory_message,                                      \
      jb::itch5::stock_trading_action_message,                                 \
      jb::itch5::system_event_message, jb::itch5::trade_message

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.load_overrides(argc, argv, std::string("mold2inside.yaml"), "JB_ROOT");
  jb::log::init(cfg.log());

  boost::asio::io_service io_service;

  boost::iostreams::filtering_ostream out;
  jb::open_output_file(out, cfg.output_file());

  std::map<jb::itch5::stock_t, jb::offline_feed_statistics> per_symbol;
  jb::offline_feed_statistics stats(cfg.stats());

  auto cb = [&stats, &cfg, &out, &per_symbol](
      jb::itch5::compute_inside::time_point recv_ts,
      jb::itch5::message_header const& header, jb::itch5::stock_t const& stock,
      jb::itch5::half_quote const& bid, jb::itch5::half_quote const& offer) {
    auto pl = std::chrono::steady_clock::now() - recv_ts;
    stats.sample(header.timestamp.ts, pl);

    if (cfg.enable_symbol_stats()) {
      auto i = per_symbol.find(stock);
      if (i == per_symbol.end()) {
        auto p = per_symbol.emplace(
            stock, jb::offline_feed_statistics(cfg.symbol_stats()));
        i = p.first;
      }
      i->second.sample(header.timestamp.ts, pl);
    }

    out << header.timestamp.ts.count() << " " << header.stock_locate << " "
        << stock << " " << bid.first.as_integer() << " " << bid.second << " "
        << offer.first.as_integer() << " " << offer.second << "\n";
  };

  jb::itch5::compute_inside handler(cb);
  auto process_buffer = [&handler](
      std::chrono::steady_clock::time_point recv_ts, std::uint64_t msgcnt,
      std::size_t msgoffset, char const* msgbuf, std::size_t msglen) {
    jb::itch5::process_buffer_mlist<jb::itch5::compute_inside,
                                    KNOWN_ITCH5_MESSAGES>::process(handler,
                                                                   recv_ts,
                                                                   msgcnt,
                                                                   msgoffset,
                                                                   msgbuf,
                                                                   msglen);
  };

  jb::itch5::mold_udp_channel channel(
      io_service, process_buffer, cfg.multicast_group(), cfg.multicast_port(),
      cfg.listen_address());

  io_service.run();

  jb::offline_feed_statistics::print_csv_header(std::cout);
  for (auto const& i : per_symbol) {
    i.second.print_csv(i.first.c_str(), std::cout);
  }
  stats.print_csv("__aggregate__", std::cout);

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

// Define the default per-symbol stats
jb::offline_feed_statistics::config default_per_symbol_stats() {
  return jb::offline_feed_statistics::config()
      .reporting_interval_seconds(24 * 3600)     // effectively disable updates
      .max_processing_latency_nanoseconds(10000) // limit memory usage
      .max_interarrival_time_nanoseconds(10000)  // limit memory usage
      .max_messages_per_microsecond(1000)        // limit memory usage
      .max_messages_per_millisecond(10000)       // limit memory usage
      .max_messages_per_second(10000)            // limit memory usage
      ;
}

std::string default_listen_address() {
  return "";
}

std::string default_multicast_group() {
  return "::1";
}

int default_multicast_port() {
  return 50000;
}

config::config()
    : multicast_port(
          desc("multicast-port").help("The multicast port to listen in."), this,
          default_multicast_port())
    , listen_address(desc("listen-address")
                         .help("The address to listen in, typically 0.0.0.0, "
                               "::, or a specific "
                               "NIC address."),
                     this, default_listen_address())
    , multicast_group(desc("multicast-group")
                          .help("The multicast group carrying the MOLD data."),
                      this, default_multicast_group())
    , output_file(
          desc("output-file")
              .help("The name of the file where to store the inside data."
                    "  Files ending in .gz are automatically compressed."),
          this)
    , log(desc("log", "logging"), this)
    , stats(desc("stats", "offline-feed-statistics"), this)
    , symbol_stats(desc("symbol-stats", "offline-feed-statistics"), this,
                   default_per_symbol_stats())
    , enable_symbol_stats(
          desc("enable-symbol-stats")
              .help(
                  "If set, enable per-symbol statistics."
                  "  Collecting per-symbol statistics is expensive in both"
                  " memory and execution time, so it is disabled by default."),
          this, false) {
}

void config::validate() const {
  if (output_file() == "") {
    throw jb::usage("Missing output-file setting."
                    "  You must specify an output file.",
                    1);
  }
  log().validate();
  stats().validate();
  symbol_stats().validate();
}

} // anonymous namespace
