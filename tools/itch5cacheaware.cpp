#include <jb/itch5/compute_book_cache_aware.hpp>
#include <jb/itch5/process_iostream.hpp>
#include <jb/book_cache_aware_stats.hpp>
#include <jb/fileio.hpp>
#include <jb/log.hpp>

#include <iostream>
#include <stdexcept>
#include <unordered_map>

/**
 * Compute ITCH5 Cache Aware statistics.
 *
 * Generate statistics per symbol and aggregated.
 * See:
 * https://github.com/GFariasR/jaybeams/wiki \
 * /ITCH5-Cache-Aware-StatisticsProject
 * for design and implementation details.
 */
namespace {

class config : public jb::config_object {
public:
  config();
  config_object_constructors(config);

  void validate() const override;

  jb::config_attribute<config, std::string> input_file;
  jb::config_attribute<config, std::string> output_file;
  jb::config_attribute<config, jb::log::config> log;
  jb::config_attribute<config, jb::book_cache_aware_stats::config> stats;
  jb::config_attribute<config, jb::book_cache_aware_stats::config> symbol_stats;
  jb::config_attribute<config, bool> enable_symbol_stats;
  jb::config_attribute<config, jb::itch5::tick_t> tick_offset;
};

} // anonymous namespace

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.load_overrides(
      argc, argv, std::string("itch5cacheaware.yaml"), "JB_ROOT");
  jb::log::init(cfg.log());

  boost::iostreams::filtering_istream in;
  jb::open_input_file(in, cfg.input_file());

  boost::iostreams::filtering_ostream out;
  jb::open_output_file(out, cfg.output_file());

  // set order book offset value
  jb::itch5::order_book_cache_aware::tick_offset(cfg.tick_offset());

  std::map<jb::itch5::stock_t, jb::book_cache_aware_stats> per_symbol;
  jb::book_cache_aware_stats stats(cfg.stats());

  auto cb =
      [&](jb::itch5::stock_t const& stock, jb::itch5::tick_t ticks,
          jb::itch5::level_t levels) {
        stats.sample(ticks, levels);

        if (cfg.enable_symbol_stats()) {
          auto i = per_symbol.find(stock);
          if (i == per_symbol.end()) {
            auto p = per_symbol.emplace(
                stock, jb::book_cache_aware_stats(cfg.symbol_stats()));
            i = p.first;
          }
          i->second.sample(ticks, levels);
        }
      };

  jb::itch5::compute_book_cache_aware handler(cb);
  jb::itch5::process_iostream(in, handler);

  jb::book_cache_aware_stats::print_csv_header(out);
  for (auto const& i : per_symbol) {
    i.second.print_csv(i.first.c_str(), out);
  }
  stats.print_csv("__aggregate__", out);
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
jb::book_cache_aware_stats::config default_per_symbol_stats() {
  return jb::book_cache_aware_stats::config().max_ticks(10000).max_levels(
      10000);
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
    , stats(desc("stats", "book-cache-aware-stats"), this)
    , symbol_stats(
          desc("symbol-stats", "book-cache-aware-stats-per-symbol"), this,
          default_per_symbol_stats())
    , enable_symbol_stats(
          desc("enable-symbol-stats")
              .help(
                  "If set, enable per-symbol statistics."
                  "  Collecting per-symbol statistics is expensive in both"
                  " memory and execution time"),
          this, true) // changes default to true
    , tick_offset(
          desc("tick-offset", "book-cache-aware-tick-offset"), this, 5000) {
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
