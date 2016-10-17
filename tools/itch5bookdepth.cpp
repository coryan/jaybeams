#include <jb/itch5/compute_book_depth.hpp>
#include <jb/itch5/process_iostream.hpp>
#include <jb/book_depth_statistics.hpp>
#include <jb/fileio.hpp>
#include <jb/log.hpp>

#include <iostream>
#include <stdexcept>
#include <unordered_map>

#include <chrono>

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
      jb::itch5::half_quote const& bid,
      jb::itch5::half_quote const& offer) {
    auto pl = std::chrono::steady_clock::now() - recv_ts;
    stats.sample(header.timestamp.ts, pl);
    
    if (cfg.enable_symbol_stats()) {
      auto i = per_symbol.find(stock);
      if (i == per_symbol.end()) {
        auto p = per_symbol.emplace(
            stock, jb::book_depth_statistics(cfg.symbol_stats()));
        i = p.first;
     }
      i->second.sample(header.timestamp.ts, pl);
    }
    
    out << header.timestamp.ts.count()
        << " " << header.stock_locate
        << " " << stock
        << " " << bid.first.as_integer()
        << " " << bid.second
        << " " << offer.first.as_integer()
        << " " << offer.second
        << "\n";
    
  };

  high_resolution_t::time_point t1 = high_resolution_t::now();
  std::cout << "working.....:  " << std::endl;

  jb::itch5::compute_book_depth handler(cb);
  jb::itch5::process_iostream(in, handler);

  std::cout << "printing... " << std::endl;
  /*
  jb::offline_feed_statistics::print_csv_header(std::cout);
  for (auto const& i : per_symbol) {
    i.second.print_csv(i.first.c_str(), std::cout);
  }
  stats.print_csv("__aggregate__", std::cout);
  */
  high_resolution_t::time_point t2 = high_resolution_t::now();
  auto duration = std::chrono::duration_cast<microseconds_t>( t2 - t1 ).count();
  std::cout << "duration:  " << duration << std::endl;

  
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
      .reporting_interval_seconds(24 * 3600) // effectively disable updates
      .max_processing_latency_nanoseconds(10000) // limit memory usage
      .max_interarrival_time_nanoseconds(10000)  // limit memory usage 
      .max_messages_per_microsecond(1000)  // limit memory usage
      .max_messages_per_millisecond(10000) // limit memory usage
      .max_messages_per_second(10000)      // limit memory usage
      ;
}

config::config()
    : input_file(desc("input-file").help(
        "An input file with ITCH-5.0 messages."), this)
    , output_file(desc("output-file").help(
        "The name of the file where to store the inside data."
        "  Files ending in .gz are automatically compressed."), this)
    , log(desc("log", "logging"), this)
    , stats(desc("stats", "offline-feed-statistics"), this)
    , symbol_stats(desc("symbol-stats", "offline-feed-statistics"),
                   this, default_per_symbol_stats())
    , enable_symbol_stats(
        desc("enable-symbol-stats").help(
            "If set, enable per-symbol statistics."
            "  Collecting per-symbol statistics is expensive in both"
            " memory and execution time, so it is disabled by default."),
        this, false)
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
