#include "jb/book_cache_aware_stats.hpp"

#include <jb/as_hhmmss.hpp>
#include <jb/log.hpp>

#include <iostream>
#include <limits>

namespace {

template <typename histogram_t>
void csv_rate(std::ostream& os, histogram_t const& histo) {
  os << histo.observed_min() << "," << histo.estimated_quantile(0.25) << ","
     << histo.estimated_quantile(0.50) << "," << histo.estimated_quantile(0.75)
     << "," << histo.estimated_quantile(0.90) << ","
     << histo.estimated_quantile(0.99) << "," << histo.estimated_quantile(0.999)
     << "," << histo.estimated_quantile(0.9999) << "," << histo.observed_max();
}

template <typename histogram_t>
void report_rate(std::chrono::nanoseconds ts, histogram_t const& histo) {
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

jb::book_cache_aware_stats::book_cache_aware_stats(config const& cfg)
    : ticks_(tick_histogram_t::binning_strategy(0, cfg.max_ticks()))
    , levels_(level_histogram_t::binning_strategy(0, cfg.max_levels())) {
}

void jb::book_cache_aware_stats::print_csv_header(std::ostream& os) {
  os << "Name,NSamples"
     << ",minTicks,p25Ticks,p50Ticks,p75Ticks"
     << ",p90Ticks,p99Ticks,p999Ticks,p9999Ticks"
     << ",maxTicks"
     << ",minLevels,p25Levels,p50Levels,p75Levels"
     << ",p90Levels,p99Levels,p999Levels,p9999Levels"
     << ",maxLevels"
     << "\n";
}

void jb::book_cache_aware_stats::print_csv(
    std::string const& name, std::ostream& os) const {
  if (ticks_.nsamples() == 0) {
    os << name << ",0";
    os << ",,,,,,,,,"; // ticks
    os << ",,,,,,,,,"; // levels
    os << "\n";
    return;
  }
  os << name << "," << ticks_.nsamples() << ",";
  csv_rate(os, ticks_);
  os << ",";
  csv_rate(os, levels_);
  os << std::endl;
}

namespace jb {
namespace defaults {

#ifndef JB_BOOK_CACHE_AWARE_STATS_DEFAULTS_max_ticks
#define JB_BOOK_CACHE_AWARE_STATS_DEFAULTS_max_ticks 8192
#endif
#ifndef JB_BOOK_CACHE_AWARE_STATS_DEFAULTS_max_levels
#define JB_BOOK_CACHE_AWARE_STATS_DEFAULTS_max_levels 8192
#endif

tick_t max_ticks = JB_BOOK_CACHE_AWARE_STATS_DEFAULTS_max_ticks;
level_t max_levels = JB_BOOK_CACHE_AWARE_STATS_DEFAULTS_max_levels;

} // namespace defaults
} // namespace jb

jb::book_cache_aware_stats::config::config()
    : max_ticks(
          desc("max-ticks")
              .help(
                  "Configure the ticks histogram to expect"
                  " no more than this many values"
                  "   Higher values consume more memory, but give more accurate"
                  " results for high percentiles."),
          this, defaults::max_ticks)
    , max_levels(
          desc("max-levels")
              .help(
                  "Configure the levels histogram to expect"
                  " no more than this many values"
                  "   Higher values consume more memory, but give more accurate"
                  " results for high percentiles."),
          this, defaults::max_levels) {
}

void jb::book_cache_aware_stats::config::validate() const {
  if (max_ticks() <= 1) {
    std::ostringstream os;
    os << "max_ticks must be > 1, value=" << max_ticks();
    throw jb::usage(os.str(), 1);
  }
  if (max_levels() <= 1) {
    std::ostringstream os;
    os << "max_levels must be > 1, value=" << max_levels();
    throw jb::usage(os.str(), 1);
  }
}
