/**
 * @file
 *
 * This program reads a raw ITCH-5.0 file and generates the inside
 * quotes in an ASCII (though potentially compressed) file.  The
 * program also generates statistics about the feed and the book
 * build, using jb::offline_feed_statistics.
 *
 * It reports the percentiles of "for each change in the inside, how
 * long did it take to process the event, and what was the elapsed
 * time since the last change to the inside".
 */
#include <jb/itch5/compute_book.hpp>
#include <jb/itch5/process_iostream.hpp>
#include <jb/fileio.hpp>
#include <jb/log.hpp>
#include <jb/offline_feed_statistics.hpp>

#include <iostream>
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

  jb::config_attribute<config, std::string> input_file;
  jb::config_attribute<config, std::string> output_file;
  jb::config_attribute<config, jb::log::config> log;
  jb::config_attribute<config, jb::offline_feed_statistics::config> stats;
  jb::config_attribute<config, jb::offline_feed_statistics::config>
      symbol_stats;
  jb::config_attribute<config, bool> enable_symbol_stats;
};

/**
 * Determine if this event changes the inside, if so, record the
 * statistics.
 *
 * @tparam duration_t the type used to record the processing latency,
 * must be compatible with a duration in the std::chrono sense.
 *
 * @param stats where to record the statistics
 * @param header (unused) the header for the message that generated
 * this book update
 * @param book the book updated
 * @param update a report of the changes to the book
 * @param processing_latency the time it took to process the event
 * (before the any output is generated).
 * @returns true if the inside is affected by the change, false otherwise.
 */
template <typename duration_t>
bool record_latency_stats(
    jb::offline_feed_statistics& stats,
    jb::itch5::message_header const& header, jb::itch5::order_book const& book,
    jb::itch5::compute_book::book_update const& update,
    duration_t processing_latency) {
  // ... we need to treat each side differently ...
  if (update.buy_sell_indicator == u'B') {
    // ... if the update price is strictly less than the current best
    // bid, that means it did not change the inside.  Notice that this
    // works even when there is no best bid, because the book returns
    // "0" as a best bid in that case ...
    if (update.px < book.best_bid().first) {
      return false;
    }
  } else {
    // ... do the analogous thing for the sell side ...
    if (update.px > book.best_offer().first) {
      return false;
    }
  }
  stats.sample(header.timestamp.ts, processing_latency);
  return true;
}

/**
 * Determine if this event changes the inside, if so, record the
 * statistics and output the result.
 *
 * @tparam duration_t the type used to record the processing latency,
 * must be compatible with a duration in the std::chrono sense.
 *
 * @param stats where to record the statistics
 * @param out where to send the new inside quote if needed
 * @param header (unused) the header for the message that generated
 * this book update
 * @param book the book updated
 * @param update a report of the changes to the book
 * @param processing_latency the time it took to process the event
 * (before the any output is generated).
 * @returns true if the inside is affected by the change, false otherwise.
 */
template <typename duration_t>
bool generate_inside(
    jb::offline_feed_statistics& stats, std::ostream& out,
    jb::itch5::message_header const& header, jb::itch5::order_book const& book,
    jb::itch5::compute_book::book_update const& update,
    duration_t processing_latency) {
  if (not record_latency_stats(
          stats, header, book, update, processing_latency)) {
    return false;
  }
  auto bid = book.best_bid();
  auto offer = book.best_offer();
  out << header.timestamp.ts.count() << " " << header.stock_locate << " "
      << update.stock << " " << bid.first.as_integer() << " " << bid.second
      << " " << offer.first.as_integer() << " " << offer.second << "\n";
  return true;
}

} // anonymous namespace

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.load_overrides(argc, argv, std::string("itch5inside.yaml"), "JB_ROOT");
  jb::log::init(cfg.log());

  boost::iostreams::filtering_istream in;
  jb::open_input_file(in, cfg.input_file());

  boost::iostreams::filtering_ostream out;
  jb::open_output_file(out, cfg.output_file());

  std::map<jb::itch5::stock_t, jb::offline_feed_statistics> per_symbol;
  jb::offline_feed_statistics stats(cfg.stats());

  jb::itch5::compute_book::callback_type cb = [&stats, &out](
      jb::itch5::message_header const& header,
      jb::itch5::order_book const& updated_book,
      jb::itch5::compute_book::book_update const& update) {
    auto pl = std::chrono::steady_clock::now() - update.recvts;
    (void) generate_inside(stats, out, header, updated_book, update, pl);
  };

  if (cfg.enable_symbol_stats()) {
    // ... replace the calback with one that also records the stats
    // for each symbol ...
    jb::offline_feed_statistics::config symcfg(cfg.symbol_stats());
    cb = [&stats, &out, &per_symbol, symcfg](
        jb::itch5::message_header const& header,
        jb::itch5::order_book const& updated_book,
        jb::itch5::compute_book::book_update const& update) {
      auto pl = std::chrono::steady_clock::now() - update.recvts;
      if (not generate_inside(stats, out, header, updated_book, update, pl)) {
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

  jb::itch5::compute_book handler(cb);
  jb::itch5::process_iostream(in, handler);

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
      .reporting_interval_seconds(24 * 3600)     // disable reporting
      .max_processing_latency_nanoseconds(10000) // limit memory usage
      .max_interarrival_time_nanoseconds(10000)  // limit memory usage
      .max_messages_per_microsecond(1000)        // limit memory usage
      .max_messages_per_millisecond(10000)       // limit memory usage
      .max_messages_per_second(10000)            // limit memory usage
      ;
}

config::config()
    : input_file(
          desc("input-file").help("An input file with ITCH-5.0 messages."),
          this)
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
  if (input_file() == "") {
    throw jb::usage(
        "Missing input-file setting."
        "  You must specify an input file.",
        1);
  }
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
