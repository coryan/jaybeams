/**
 * @file
 *
 * This is a benchmark for jb::itch5::array_based_order_book and
 * jb::itch5::map_based_order_book.  It runs the order book
 * computation over a portion of an ITCH-5.0 file, typically the
 * program is driven by a script or by cachegrind to collect
 * performance information.
 */
#include <jb/itch5/array_based_order_book.hpp>
#include <jb/itch5/compute_book.hpp>
#include <jb/itch5/map_based_order_book.hpp>
#include <jb/itch5/process_iostream.hpp>
#include <jb/fileio.hpp>
#include <jb/histogram.hpp>
#include <jb/integer_range_binning.hpp>
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

  using array_config = typename jb::itch5::array_based_order_book::config;
  using map_config = typename jb::itch5::map_based_order_book::config;

  jb::config_attribute<config, std::string> input_file;
  jb::config_attribute<config, jb::log::config> log;
  jb::config_attribute<config, bool> enable_array_based;
  jb::config_attribute<config, array_config> array_book;
  jb::config_attribute<config, map_config> map_book;
  jb::config_attribute<config, int> stop_after_seconds;
  jb::config_attribute<config, int> max_processing_latency_nanoseconds;
};

/// A simple struct to signal termination of the
/// jb::itch5::process_iostream() loop
struct abort_process_iostream {};

} // anonymous namespace

/**
 * Run the benchmark for a specific book type
 *
 * @param cfg the configuration for the benchmark
 * @param book_cfg the configuration for the specific book type
 *
 * @tparam book_type the type of book to use in the benchmark
 * @tparam book_type_config the configuration class for @a book_type
 */
template <typename book_type, typename book_type_config>
void run_inside(config const& cfg, book_type_config const& book_cfg) {
  // initialize the logging framework ...
  jb::log::init(cfg.log());

  // ... setup the input file, automatically determines if
  // uncompression is necessary based on the filename ...
  boost::iostreams::filtering_istream in;
  jb::open_input_file(in, cfg.input_file());

  // ... create a histogram to capture latency statistics, the main
  // point of the benchmark after all ...
  using latency_histogram =
      jb::histogram<jb::integer_range_binning<std::int64_t>>;
  latency_histogram histo(
      latency_histogram::binning_strategy(
          0, cfg.max_processing_latency_nanoseconds()));

  // ... convert the configuration parameter into a duration type ...
  std::chrono::seconds stop_after(cfg.stop_after_seconds());

  // ... create a callback of the right type ...
  using callback_type =
      typename jb::itch5::compute_book<book_type>::callback_type;
  callback_type cb = [&histo, stop_after](
      jb::itch5::message_header const& header,
      jb::itch5::order_book<book_type> const& updated_book,
      jb::itch5::book_update const& update) {
    using namespace std::chrono;
    if (stop_after != seconds(0) and stop_after <= header.timestamp.ts) {
      throw abort_process_iostream{};
    }
    auto pl = steady_clock::now() - update.recvts;
    histo.sample(duration_cast<nanoseconds>(pl).count());
  };

  jb::itch5::compute_book<book_type> handler(cb, book_cfg);
  try {
    jb::itch5::process_iostream(in, handler);
  } catch (abort_process_iostream const&) {
    // nothing to do, the loop is terminated by the exception and we
    // continue the code ...
    JB_LOG(info) << "process_iostream aborted, stop_after_seconds="
                 << cfg.stop_after_seconds();
  }
  JB_LOG(info) << "processing latency (ns): " << histo.summary();
}

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.load_overrides(argc, argv, std::string("bm_order_book.yaml"), "JB_ROOT");

  // If configured, use the array based order book and its configuration ...
  if (cfg.enable_array_based()) {
    run_inside<jb::itch5::array_based_order_book>(cfg, cfg.array_book());
    return 0;
  }
  // otherwise, use a map based order book, there is nothing to configure
  run_inside<jb::itch5::map_based_order_book>(cfg, cfg.map_book());

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
#ifndef JB_ITCH5_DEFAULT_bm_order_book_stop_after_seconds
#define JB_ITCH5_DEFAULT_bm_order_book_stop_after_seconds (9 * 60 + 35) * 60
#endif // JB_ITCH5_DEFAULT_bm_order_book_stop_after_seconds

#ifndef JB_ITCH5_DEFAULT_bm_order_book_max_processing_latency
#define JB_ITCH5_DEFAULT_bm_order_book_max_processing_latency 1000000
#endif // JB_ITCH5_DEFAULT_bm_order_book_max_processing_latency

int constexpr stop_after_seconds =
    JB_ITCH5_DEFAULT_bm_order_book_stop_after_seconds;
int constexpr max_processing_latency_nanoseconds =
    JB_ITCH5_DEFAULT_bm_order_book_max_processing_latency;
} // namespace defaults

config::config()
    : input_file(
          desc("input-file").help("An input file with ITCH-5.0 messages."),
          this)
    , log(desc("log", "logging"), this)
    , enable_array_based(
          desc("enable-array-based")
              .help(
                  "If set, enable array_based_order_book usage."
                  " It is disabled by default."),
          this, false)
    , array_book(desc("array-book"), this)
    , map_book(desc("map-book"), this)
    , stop_after_seconds(
          desc("stop-after-seconds")
              .help(
                  "If non-zero, stop processing the input after this many "
                  "seconds in the input.  For example, if set to 34500 (= 9 * "
                  "3600 + 35 * 60) the processing will stop when the first "
                  "event timestamped after 09:35:00 is received."),
          this, defaults::stop_after_seconds)
    , max_processing_latency_nanoseconds(
          desc("max-processing-time-nanoseconds")
              .help(
                  "Configure the processing latency histogram to expect"
                  " no more that no processing time is higher than this value."
                  "   Higher values consume more memory, but give more accurate"
                  " results for high percentiles."),
          this, defaults::max_processing_latency_nanoseconds) {
}

void config::validate() const {
  if (input_file() == "") {
    throw jb::usage(
        "Missing input-file setting."
        "  You must specify an input file.",
        1);
  }
  if (stop_after_seconds() < 0) {
    throw jb::usage("The stop-after-seconds must be >= 0", 1);
  }
  if (max_processing_latency_nanoseconds() < 0) {
    throw jb::usage("The max-processing-latency-nanoseconds must be >= 0", 1);
  }

  log().validate();
  array_book().validate();
  map_book().validate();
}

} // anonymous namespace
