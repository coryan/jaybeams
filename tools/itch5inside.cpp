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
#include <jb/itch5/generate_inside.hpp>
#include <jb/itch5/process_iostream.hpp>
#include <jb/fileio.hpp>
#include <jb/log.hpp>

#include <stdexcept>
#include <type_traits>
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
  jb::config_attribute<config, bool> enable_array_based;
  jb::config_attribute<config,
                       typename jb::itch5::array_based_order_book::config>
      book_cfg;
};

} // anonymous namespace

template <typename book_type_t, typename cfg_book_t>
void run_inside(
    config const& cfg, book_type_t const& bk, cfg_book_t const& cfg_book) {
  jb::log::init(cfg.log());

  boost::iostreams::filtering_istream in;
  jb::open_input_file(in, cfg.input_file());

  boost::iostreams::filtering_ostream out;
  jb::open_output_file(out, cfg.output_file());

  std::map<jb::itch5::stock_t, jb::offline_feed_statistics> per_symbol;
  jb::offline_feed_statistics stats(cfg.stats());

  typename jb::itch5::compute_book<book_type_t>::callback_type cb = [&stats,
                                                                     &out](
      jb::itch5::message_header const& header,
      jb::itch5::order_book<book_type_t> const& updated_book,
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
        jb::itch5::order_book<book_type_t> const& updated_book,
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

  jb::itch5::compute_book<book_type_t> handler(cb, cfg_book);
  jb::itch5::process_iostream(in, handler);

  jb::offline_feed_statistics::print_csv_header(std::cout);
  for (auto const& i : per_symbol) {
    i.second.print_csv(i.first.c_str(), std::cout);
  }
  stats.print_csv("__aggregate__", std::cout);
}

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.load_overrides(argc, argv, std::string("itch5inside.yaml"), "JB_ROOT");

  if (cfg.enable_array_based()) {
    jb::itch5::array_based_order_book book;
    (void)run_inside(cfg, book, cfg.book_cfg());
  } else {
    jb::itch5::map_based_order_book book;
    typename jb::itch5::map_based_order_book::config cfg_bk;
    (void)run_inside(cfg, book, cfg_bk);
  }

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
          this, false)
    , enable_array_based(
          desc("enable-array-based")
              .help(
                  "If set, enable array_based_order_book usage."
                  " It is disabled by default."),
          this, false)
    , book_cfg(desc("book-config", "order-book-config"), this) {
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
  book_cfg().validate();
}

} // anonymous namespace
