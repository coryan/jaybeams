#include "jb/book_depth_statistics.hpp"

#include <jb/as_hhmmss.hpp>
#include <jb/log.hpp>

#include <iostream>
#include <limits>

namespace {

template <typename book_depth_histogram_t>
void csv_rate(std::ostream& os, book_depth_histogram_t const& histo) {
  os << histo.observed_min() << "," << histo.estimated_quantile(0.25) << ","
     << histo.estimated_quantile(0.50) << "," << histo.estimated_quantile(0.75)
     << "," << histo.estimated_quantile(0.90) << ","
     << histo.estimated_quantile(0.99) << "," << histo.estimated_quantile(0.999)
     << "," << histo.estimated_quantile(0.9999) << "," << histo.observed_max();
}

template <typename book_depth_histogram_t>
void report_rate(
    std::chrono::nanoseconds ts, book_depth_histogram_t const& histo) {
  JB_LOG(info) << ": " << jb::as_hhmmss(ts)
               << ", NSamples =" << histo.nsamples()
               << ", min=" << histo.observed_min()
               << ", p25=" << histo.estimated_quantile(0.25)
               << ", p50=" << histo.estimated_quantile(0.50)
               << ", p75=" << histo.estimated_quantile(0.75)
               << ", p90=" << histo.estimated_quantile(0.90)
               << ", p99=" << histo.estimated_quantile(0.99)
               << ", p99.9=" << histo.estimated_quantile(0.999)
               << ", p99.99=" << histo.estimated_quantile(0.9999)
               << ", max=" << histo.observed_max();
}

} // anonymous namespace

jb::book_depth_statistics::book_depth_statistics(config const& cfg)
    : book_depth_(
          book_depth_histogram_t::binning_strategy(0, cfg.max_book_depth())) {
}

void jb::book_depth_statistics::print_csv_header(std::ostream& os) {
  os << "Name,NSamples"
     << ",minBookDepth,p25BookDepth,p50BookDepth,p75BookDepth"
     << ",p90BookDepth,p99BookDepth,p999BookDepth,p9999BookDepth"
     << ",maxBookDepth"
     << "\n";
}

void jb::book_depth_statistics::print_csv(
    std::string const& name, std::ostream& os) const {
  if (book_depth_.nsamples() == 0) {
    os << name << ",0";
    os << ",,,,,,,,,"; // book depth
    os << "\n";
    return;
  }
  os << name << "," << book_depth_.nsamples() << ",";
  csv_rate(os, book_depth_);
  os << std::endl;
}

namespace jb {
namespace defaults {

#ifndef JB_BOOK_DEPTH_STATS_DEFAULTS_max_book_depth
#define JB_BOOK_DEPTH_STATS_DEFAULTS_max_book_depth 8192
#endif

book_depth_t max_book_depth = JB_BOOK_DEPTH_STATS_DEFAULTS_max_book_depth;

} // namespace defaults
} // namespace jb

jb::book_depth_statistics::config::config()
    : max_book_depth(
          desc("max-book-depth")
              .help(
                  "Configure the book_depth histogram to expect"
                  " no more than this many values"
                  "   Higher values consume more memory, but give more accurate"
                  " results for high percentiles."),
          this, defaults::max_book_depth) {
}

void jb::book_depth_statistics::config::validate() const {
  if (max_book_depth() <= 1) {
    std::ostringstream os;
    os << "max_book_depth must be > 1, value=" << max_book_depth();
    throw jb::usage(os.str(), 1);
  }
}
