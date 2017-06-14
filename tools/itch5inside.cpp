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
  using book_config = typename jb::itch5::array_based_order_book::config;
  jb::config_attribute<config, book_config> book_cfg;

  jb::config_attribute<config, int> stop_after_seconds;
};

/// A simple struct to signal termination of the
/// jb::itch5::process_iostream() loop
struct abort_process_iostream {};

} // anonymous namespace

/**
 * Template function to refactor the usage of a book side type.
 *
 * @tparam book_type_t based order book type
 * @tparam cfg_book_t Defines the config type

 * @param cfg Application config object
 * @param cfg_book Book side config object
 */
template <typename book_type_t, typename cfg_book_t>
void run_inside(config const& cfg, cfg_book_t const& cfg_book) {
  jb::log::init(cfg.log());

  boost::iostreams::filtering_istream in;
  jb::open_input_file(in, cfg.input_file());

  boost::iostreams::filtering_ostream out;
  jb::open_output_file(out, cfg.output_file());

  std::map<jb::itch5::stock_t, jb::offline_feed_statistics> per_symbol;
  jb::offline_feed_statistics stats(cfg.stats());

  std::chrono::seconds stop_after(cfg.stop_after_seconds());

  using callback_type =
      typename jb::itch5::compute_book<book_type_t>::callback_type;
  callback_type cb = [&stats, &out, stop_after](
      jb::itch5::message_header const& header,
      jb::itch5::order_book<book_type_t> const& updated_book,
      jb::itch5::book_update const& update) {
    if (stop_after != std::chrono::seconds(0) and
        stop_after <= header.timestamp.ts) {
      throw abort_process_iostream{};
    }
    auto pl = std::chrono::steady_clock::now() - update.recvts;
    (void)jb::itch5::generate_inside(
        stats, out, header, updated_book, update, pl);
  };

  if (cfg.enable_symbol_stats()) {
    // ... replace the calback with one that also records the stats
    // for each symbol ...
    jb::offline_feed_statistics::config symcfg(cfg.symbol_stats());
    cb = [&stats, &out, &per_symbol, symcfg, stop_after](
        jb::itch5::message_header const& header,
        jb::itch5::order_book<book_type_t> const& updated_book,
        jb::itch5::book_update const& update) {
      if (stop_after != std::chrono::seconds(0) and
          stop_after <= header.timestamp.ts) {
        throw abort_process_iostream{};
      }
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

  jb::itch5::compute_book<book_type_t> handler(std::move(cb), cfg_book);
  try {
    jb::itch5::process_iostream(in, handler);
  } catch (abort_process_iostream const&) {
    // nothing to do, the loop is terminated by the exception and we
    // continue the code ...
    JB_LOG(info) << "process_iostream aborted, stop_after_seconds="
                 << cfg.stop_after_seconds();
  }
  stats.log_final_progress();

  jb::offline_feed_statistics::print_csv_header(std::cout);
  for (auto const& i : per_symbol) {
    i.second.print_csv(i.first.c_str(), std::cout);
  }
  stats.print_csv("__aggregate__", std::cout);
}

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.load_overrides(argc, argv, std::string("itch5inside.yaml"), "JB_ROOT");

  /// if enable_array_based uses array_based_order_book type
  /// and the config built by the call's arguments
  if (cfg.enable_array_based()) {
    (void)run_inside<jb::itch5::array_based_order_book>(cfg, cfg.book_cfg());
  } else {
    /// ... uses map_based_order_book type and a default config
    typename jb::itch5::map_based_order_book::config cfg_bk;
    (void)run_inside<jb::itch5::map_based_order_book>(cfg, cfg_bk);
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
    , book_cfg(desc("book-config", "order-book-config"), this)
    , stop_after_seconds(
          desc("stop-after-seconds")
              .help(
                  "If non-zero, stop processing the input after this many "
                  "seconds in the input.  For example, if set to 34500 (= 9 * "
                  "3600 + 35 * 60) the processing will stop when the first "
                  "event timestamped after 09:35:00 is received."),
          this, 0) {
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
  if (stop_after_seconds() < 0) {
    throw jb::usage("The stop-after-seconds must be >= 0", 1);
  }

  log().validate();
  stats().validate();
  symbol_stats().validate();
  book_cfg().validate();
}

} // anonymous namespace
