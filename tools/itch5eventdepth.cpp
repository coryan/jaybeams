/**
 * @file
 *
 * This program tries to answer the question:
 * - How many changes happen at the top of book?
 * - How many changes happen 2 levels down the book?
 * - What about 10 levels?
 *
 * It reports the percentiles of "for each event, record the depth of
 * the change".
 */
#include <jb/itch5/compute_book.hpp>
#include <jb/itch5/price_levels.hpp>
#include <jb/itch5/process_iostream.hpp>
#include <jb/book_depth_statistics.hpp>
#include <jb/fileio.hpp>
#include <jb/log.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <unordered_map>

/**
 * Define types and functions used in this program.
 */
namespace {

/// Configuration parameters for itch5eventdepth
class config : public jb::config_object {
public:
  config();
  config_object_constructors(config);

  void validate() const override;

  jb::config_attribute<config, std::string> input_file;
  jb::config_attribute<config, std::string> output_file;
  jb::config_attribute<config, jb::log::config> log;
  jb::config_attribute<config, jb::book_depth_statistics::config> stats;
  jb::config_attribute<config, jb::book_depth_statistics::config> symbol_stats;
  jb::config_attribute<config, bool> enable_symbol_stats;
};

/// Calculate the depth of an event, taking care with events that
/// moved the BBO.
void record_event_depth(
    jb::book_depth_statistics& stats, jb::itch5::message_header const&,
    jb::itch5::order_book const& book,
    jb::itch5::compute_book::book_update const& update) {
  // ... we need to treat each side differently ...
  int depth = 0;
  if (update.buy_sell_indicator == u'B') {
    // ... if the update price is higher than the current best bid
    // that means the update moved the best_bid down, we are going to
    // treat that as a change at depth 0.  Notice that this works even
    // when there is no best bid, because the book returns "0" as a
    // best bid in that case ...
    if (update.px < book.best_bid().first) {
      depth = jb::itch5::price_levels(update.px, book.best_bid().first);
    }
  } else {
    // ... do the analogous thing for the sell side ...
    if (update.px > book.best_offer().first) {
      depth = jb::itch5::price_levels(book.best_offer().first, update.px);
    }
  }
  stats.sample(depth);
}

} // anonymous namespace

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.load_overrides(
      argc, argv, std::string("itch5eventdepth.yaml"), "JB_ROOT");
  jb::log::init(cfg.log());

  boost::iostreams::filtering_istream in;
  jb::open_input_file(in, cfg.input_file());

  boost::iostreams::filtering_ostream out;
  jb::open_output_file(out, cfg.output_file());

  std::map<jb::itch5::stock_t, jb::book_depth_statistics> per_symbol;
  jb::book_depth_statistics aggregate_stats(cfg.stats());

  jb::itch5::compute_book::callback_type cb = [&aggregate_stats](
      jb::itch5::message_header const& header,
      jb::itch5::order_book const& updated_book,
      jb::itch5::compute_book::book_update const& update) {
    record_event_depth(aggregate_stats, header, updated_book, update);
  };

  if (cfg.enable_symbol_stats()) {
    jb::book_depth_statistics::config symcfg(cfg.symbol_stats());
    jb::itch5::compute_book::callback_type chain = [&per_symbol, symcfg, cb](
        jb::itch5::message_header const& header,
        jb::itch5::order_book const& book,
        jb::itch5::compute_book::book_update const& update) {
      cb(header, book, update);
      auto location = per_symbol.find(update.stock);
      if (location == per_symbol.end()) {
        auto p =
            per_symbol.emplace(update.stock, jb::book_depth_statistics(symcfg));
        location = p.first;
      }
      record_event_depth(location->second, header, book, update);
    };
    cb = std::move(chain);
  }

  jb::itch5::compute_book handler(std::move(cb));
  jb::itch5::process_iostream(in, handler);

  jb::book_depth_statistics::print_csv_header(out);
  for (auto const& i : per_symbol) {
    i.second.print_csv(i.first.c_str(), out);
  }
  aggregate_stats.print_csv("__aggregate__", out);
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

/// Limit the amount of memory used on each per-symbol statistics
#ifndef JB_ITCH5EVENTDEPTH_DEFAULT_per_symbol_max_book_depth
#define JB_ITCH5EVENTDEPTH_DEFAULT_per_symbol_max_book_depth 5000
#endif // JB_ITCH5EVENTDEPTH_DEFAULT_per_symbol_max_book_depth

/// Create a different default configuration for the per-symbol stats
jb::book_depth_statistics::config default_per_symbol_stats() {
  return jb::book_depth_statistics::config().max_book_depth(
      JB_ITCH5EVENTDEPTH_DEFAULT_per_symbol_max_book_depth);
}

config::config()
    : input_file(
          desc("input-file").help("An input file with ITCH-5.0 messages."),
          this)
    , output_file(
          desc("output-file")
              .help(
                  "The name of the file where to store the statistics."
                  "  By default output to stdout."
                  "  Files ending in .gz are automatically compressed."),
          this, "stdout")
    , log(desc("log", "logging"), this)
    , stats(desc("stats", "event-depth-statistics"), this)
    , symbol_stats(
          desc("symbol-stats", "event-depth-statistics-per-symbol"), this,
          default_per_symbol_stats())
    , enable_symbol_stats(
          desc("enable-symbol-stats")
              .help(
                  "If set, enable per-symbol statistics."
                  "  Collecting per-symbol statistics is expensive in both"
                  " memory and execution time, enable only if needed."),
          this, true) {
}

void config::validate() const {
  if (input_file() == "") {
    throw jb::usage(
        "Missing input-file setting."
        "  The program needs an input file to read ITCH-5.0 data from.",
        1);
  }
  if (output_file() == "") {
    throw jb::usage(
        "Missing output-file setting."
        "  Use 'stdout' if you want to print to the standard output.",
        1);
  }
  log().validate();
  stats().validate();
  symbol_stats().validate();
}

} // anonymous namespace
