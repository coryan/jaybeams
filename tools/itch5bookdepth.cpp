/**
 * @file
 *
 * Compute ITCH5 Depth of Book statistics.
 *
 * Generate statistics per symbol and aggregated.
 * See:
 * https://github.com/GFariasR/jaybeams/wiki/ITCH5-Depth-of-Book-StatisticsProject
 * for design and implementation details.
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

/// Configuration parameters for itch5bookdepth
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

/// Record the book depth
void record_book_depth(
    jb::book_depth_statistics& stats, jb::itch5::message_header const&,
    jb::itch5::order_book const& book,
    jb::itch5::compute_book::book_update const& update) {
  auto buy_price_levels =
      jb::itch5::price_levels(book.worst_bid().first, book.best_bid().first);
  auto sell_price_levels = jb::itch5::price_levels(
      book.best_offer().first, book.worst_offer().first);
  stats.sample(buy_price_levels + sell_price_levels);
}

} // anonymous namespace

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.load_overrides(argc, argv, std::string("itch5bookdepth.yaml"), "JB_ROOT");
  jb::log::init(cfg.log());

  boost::iostreams::filtering_istream in;
  jb::open_input_file(in, cfg.input_file());

  boost::iostreams::filtering_ostream out;
  jb::open_output_file(out, cfg.output_file());

  std::map<jb::itch5::stock_t, jb::book_depth_statistics> per_symbol;
  jb::book_depth_statistics stats(cfg.stats());

  jb::itch5::compute_book::callback_type cb = [&stats](
      jb::itch5::message_header const& header,
      jb::itch5::order_book const& updated_book,
      jb::itch5::compute_book::book_update const& update) {
    record_book_depth(stats, header, updated_book, update);
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
      record_book_depth(location->second, header, book, update);
    };
    cb = std::move(chain);
  }

  jb::itch5::compute_book handler(std::move(cb));
  jb::itch5::process_iostream(in, handler);

  jb::book_depth_statistics::print_csv_header(out);
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

/// Limit the amount of memory used on each per-symbol statistics
#ifndef JB_ITCH5BOOKDEPTH_DEFAULT_per_symbol_max_book_depth
#define JB_ITCH5BOOKDEPTH_DEFAULT_per_symbol_max_book_depth 5000
#endif // JB_ITCH5BOOKDEPTH_DEFAULT_per_symbol_max_book_depth

/// Create a different default configuration for the per-symbol stats
jb::book_depth_statistics::config default_per_symbol_stats() {
  return jb::book_depth_statistics::config().max_book_depth(
      JB_ITCH5BOOKDEPTH_DEFAULT_per_symbol_max_book_depth);
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
    , stats(desc("stats", "book-depth-statistics"), this)
    , symbol_stats(
          desc("symbol-stats", "book-depth-statistics-per-symbol"), this,
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
