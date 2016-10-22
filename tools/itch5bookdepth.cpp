#include <jb/itch5/compute_book_depth.hpp>
#include <jb/itch5/process_iostream.hpp>
#include <jb/book_depth_statistics.hpp>
#include <jb/fileio.hpp>
#include <jb/log.hpp>

#include <iostream>
#include <stdexcept>
#include <unordered_map>

#include <chrono>

/* Ticket https://github.com/coryan/jaybeams/issues/20
 *
 * See https://github.com/GFariasR/jaybeams/wiki/ITCH5-Depth-of-Book-StatisticsProject
 * for design details
 *
 */

namespace {

class config : public jb::config_object {
 public:
  config();
  config_object_constructors(config);

  void validate() const override;

  jb::config_attribute<config,std::string> input_file;
  jb::config_attribute<config,std::string> output_file;
  jb::config_attribute<config,jb::log::config> log;
  jb::config_attribute<config,jb::book_depth_statistics::config> stats;
  jb::config_attribute<config,jb::book_depth_statistics::config> symbol_stats;
  jb::config_attribute<config,bool> enable_symbol_stats;
};

} // anonymous namespace

typedef std::chrono::high_resolution_clock high_resolution_t;
typedef std::chrono::microseconds microseconds_t;

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.load_overrides(
      argc, argv, std::string("itch5bookdepth.yaml"), "JB_ROOT");
  jb::log::init(cfg.log());

  boost::iostreams::filtering_istream in;
  jb::open_input_file(in, cfg.input_file());

  boost::iostreams::filtering_ostream out;
  jb::open_output_file(out, cfg.output_file());

  std::map<jb::itch5::stock_t, jb::book_depth_statistics> per_symbol;
  jb::book_depth_statistics stats(cfg.stats());

  auto cb = [&](
      jb::itch5::compute_book_depth::time_point recv_ts,
      jb::itch5::message_header const& header,
      jb::itch5::stock_t const& stock,
      jb::itch5::book_depth_t const& book_depth) {
    auto pl = std::chrono::steady_clock::now() - recv_ts;
    stats.sample(header.timestamp.ts, book_depth);
    
    if (cfg.enable_symbol_stats()) {
      auto i = per_symbol.find(stock);
      if (i == per_symbol.end()) {
        auto p = per_symbol.emplace(
            stock, jb::book_depth_statistics(cfg.symbol_stats()));
        i = p.first;
     }
      i->second.sample(header.timestamp.ts, book_depth);
    } 
    out << header.timestamp.ts.count()
        << " " << header.stock_locate
        << " " << stock
        << " " << book_depth
        << "\n";  
  };
    
  jb::itch5::compute_book_depth handler(cb);
  jb::itch5::process_iostream(in, handler);

  // now print the stats per simbol, and aggregated to output file
  jb::book_depth_statistics::print_csv_header(out);
  for (auto const& i : per_symbol) {
    i.second.print_csv(i.first.c_str(), out);
  }
  stats.print_csv("__aggregate__", out);
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

// Define the default per-symbol stats
jb::book_depth_statistics::config default_per_symbol_stats() {
  return jb::book_depth_statistics::config()
    .max_book_depth(10000)      // limit memory usage
      ;
}

config::config()
    : input_file(desc("input-file").help(
        "An input file with ITCH-5.0 messages."), this)
    , output_file(desc("output-file").help(
        "The name of the file where to store the inside data."
        "  Files ending in .gz are automatically compressed."), this)
    , log(desc("log", "logging"), this)
    , stats(desc("stats", "book-depth-statistics"), this)
    , symbol_stats(desc("symbol-stats", "book-depth-statistics-per-symbol"),
                   this, default_per_symbol_stats())
    , enable_symbol_stats(
        desc("enable-symbol-stats").help(
            "If set, enable per-symbol statistics."
            "  Collecting per-symbol statistics is expensive in both"
            " memory and execution time"),
        this, true)      // changes default to true
{}

void config::validate() const {
  if (input_file() == "") {
    throw jb::usage(
        "Missing input-file setting."
        "  You must specify an input file.", 1);
  }
  if (output_file() == "") {
    throw jb::usage(
        "Missing output-file setting."
        "  You must specify an output file.", 1);
  }
  log().validate();
  stats().validate();
  symbol_stats().validate();
}

} // anonymous namespace
