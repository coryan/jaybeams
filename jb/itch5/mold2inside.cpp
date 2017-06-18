/**
 * @file
 *
 * This program receives MoldUDP64 packets containing ITCH-5.0 messges
 * and generates the inside quotes in an ASCII (though potentially
 * compressed) file.  The program also generates statistics about the
 * feed and the book build, using jb::offline_feed_statistics.
 *
 * It reports the percentiles of "for each change in the inside, how
 * long did it take to process the event, and what was the elapsed
 * time since the last change to the inside".
 */
#include <jb/itch5/generate_inside.hpp>
#include <jb/itch5/mold_udp_channel.hpp>
#include <jb/itch5/process_iostream.hpp>
#include <jb/itch5/udp_receiver_config.hpp>
#include <jb/fileio.hpp>
#include <jb/log.hpp>

#include <stdexcept>
#include <unordered_map>

/**
 * Define types and functions used in this program.
 */
namespace {

/// Configuration parameters for itch5inside
class config : public jb::config_object {
public:
  config();
  config_object_constructors(config);

  void validate() const override;

  jb::config_attribute<config, jb::itch5::udp_receiver_config> receiver;
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

  jb::itch5::compute_book<jb::itch5::map_based_order_book>::callback_type cb =
      [&stats, &out](
          jb::itch5::message_header const& header,
          jb::itch5::order_book<jb::itch5::map_based_order_book> const&
              updated_book,
          jb::itch5::book_update const& update) {
        auto pl = std::chrono::steady_clock::now() - update.recvts;
        (void)jb::itch5::generate_inside(
            stats, out, header, updated_book, update, pl);
      };
  if (cfg.enable_symbol_stats()) {
    // ... replace the calback with one that also records the stats
    // for each symbol ...
    jb::offline_feed_statistics::config symcfg(cfg.symbol_stats());
    cb = [&stats, &out, &per_symbol, symcfg](
        jb::itch5::message_header const& header,
        jb::itch5::order_book<jb::itch5::map_based_order_book> const&
            updated_book,
        jb::itch5::book_update const& update) {
      auto pl = std::chrono::steady_clock::now() - update.recvts;
      if (not jb::itch5::generate_inside(
              stats, out, header, updated_book, update, pl)) {
        return;
      }
      auto location = per_symbol.find(update.stock);
      if (location == per_symbol.end()) {
        auto p = per_symbol.emplace(
            update.stock, jb::offline_feed_statistics(symcfg));
        location = p.first;
      }
      location->second.sample(header.timestamp.ts, pl);
    };
  }

  typename jb::itch5::map_based_order_book::config cfg_bk;
  jb::itch5::compute_book<jb::itch5::map_based_order_book> handler(cb, cfg_bk);
  auto process_buffer = [&handler](
      std::chrono::steady_clock::time_point recv_ts, std::uint64_t msgcnt,
      std::size_t msgoffset, char const* msgbuf, std::size_t msglen) {
    jb::itch5::process_buffer_mlist<
        jb::itch5::compute_book<jb::itch5::map_based_order_book>,
        KNOWN_ITCH5_MESSAGES>::
        process(handler, recv_ts, msgcnt, msgoffset, msgbuf, msglen);
  };

  jb::itch5::mold_udp_channel channel(
      io_service, std::move(process_buffer), cfg.receiver());

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

namespace defaults {
// Define the default per-symbol stats
jb::offline_feed_statistics::config per_symbol_stats() {
  return jb::offline_feed_statistics::config()
      .reporting_interval_seconds(24 * 3600)     // effectively disable updates
      .max_processing_latency_nanoseconds(10000) // limit memory usage
      .max_interarrival_time_nanoseconds(10000)  // limit memory usage
      .max_messages_per_microsecond(1000)        // limit memory usage
      .max_messages_per_millisecond(10000)       // limit memory usage
      .max_messages_per_second(10000)            // limit memory usage
      ;
}

std::string const local_address = "";
std::string const address = "::1";
int const port = 50000;
} // namespace defaults

config::config()
    : receiver(
          desc("receiver"), this,
          jb::itch5::udp_receiver_config()
              .port(defaults::port)
              .local_address(defaults::local_address)
              .address(defaults::address))
    , output_file(
          desc("output-file")
              .help(
                  "The name of the file where to store the inside data."
                  "  Files ending in .gz are automatically compressed."),
          this)
    , log(desc("log", "logging"), this)
    , stats(desc("stats", "offline-feed-statistics"), this)
    , symbol_stats(
          desc("symbol-stats", "offline-feed-statistics"), this,
          defaults::per_symbol_stats())
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
    throw jb::usage(
        "Missing output-file setting."
        "  You must specify an output file.",
        1);
  }
  log().validate();
  stats().validate();
  symbol_stats().validate();
}

} // anonymous namespace
