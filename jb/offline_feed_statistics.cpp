#include <jb/offline_feed_statistics.hpp>

#include <iostream>

jb::offline_feed_statistics::offline_feed_statistics(config const& cfg)
    : per_sec_rate_(
        cfg.max_messages_per_second(), std::chrono::seconds(1))
    , per_msec_rate_(
        cfg.max_messages_per_millisecond(), std::chrono::milliseconds(1))
    , per_usec_rate_(
        cfg.max_messages_per_microsecond(), std::chrono::microseconds(1))
    , interarrival_(interarrival_histogram_t::binning_strategy(
        0, cfg.max_interarrival_time_nanoseconds()))
    , processing_latency_(processing_latency_histogram_t::binning_strategy(
        0, cfg.max_processing_latency_nanoseconds()))
    , reporting_interval_(
        std::chrono::seconds(cfg.reporting_interval_seconds()))
    , last_ts_(0)
{}

void jb::offline_feed_statistics::print_csv_header(std::ostream& os) {
  char const* fields[] = {
    "min", "p25", "p50", "p75", "p90", "p99", "p999", "p9999", "max" };
  char const* tracked[] = {
    "RatePerSec", "RatePerMSec", "RatePerUSec", "Arrival",
    "ProcessingLatency" };
  os << "Name";
  for (auto t : tracked) {
    for (auto f : fields) {
      os << "," << f << t;
    }
  }
}

void jb::offline_feed_statistics::print_csv(
    std::string const& name, std::ostream& os) const {
  if (processing_latency_.nsamples() == 0) {
    os << name << ",0";
    os << ",,,,,,,,,,"; // per-second rate
    os << ",,,,,,,,,,"; // per-millisecond rate
    os << ",,,,,,,,,,"; // per-microsecond rate
    os << ",,,,,,,,,,"; // interarrival
    os << ",,,,,,,,,,"; // processing latency
    return;
  }
}

namespace jb {
namespace defaults {

#ifndef JB_OFS_DEFAULTS_max_messages_per_second
#define JB_OFS_DEFAULTS_max_messages_per_second 1000000
#endif
#ifndef JB_OFS_DEFAULTS_max_messages_per_millisecond
#define JB_OFS_DEFAULTS_max_messages_per_millisecond 100000
#endif
#ifndef JB_OFS_DEFAULTS_max_messages_per_microsecond
#define JB_OFS_DEFAULTS_max_messages_per_microsecond 100000
#endif
#ifndef JB_OFS_DEFAULTS_max_interarrival_time_nanoseconds
#define JB_OFS_DEFAULTS_max_interarrival_time_nanoseconds 100000
#endif
#ifndef JB_OFS_DEFAULTS_max_processing_latency_nanoseconds
#define JB_OFS_DEFAULTS_max_processing_latency_nanoseconds 1000000
#endif
#ifndef JB_OFS_DEFAULTS_reporting_interval_seconds
#define JB_OFS_DEFAULTS_reporting_interval_seconds 600
#endif

int max_messages_per_second = JB_OFS_DEFAULTS_max_messages_per_second;
int max_messages_per_millisecond = JB_OFS_DEFAULTS_max_messages_per_millisecond;
int max_messages_per_microsecond = JB_OFS_DEFAULTS_max_messages_per_microsecond;
std::int64_t max_interarrival_time_nanoseconds =
    JB_OFS_DEFAULTS_max_interarrival_time_nanoseconds;
int max_processing_latency_nanoseconds =
    JB_OFS_DEFAULTS_max_processing_latency_nanoseconds;
int reporting_interval_seconds =
    JB_OFS_DEFAULTS_reporting_interval_seconds;

} // namespace defaults
} // namespace jb

jb::offline_feed_statistics::config::config()
    : max_messages_per_second(
        desc("max-messages-per-second").help(
            "Configure the per-second messages rate histogram to expect"
            " no more than this number of messages per second."
            "  Higher values consume more memory, but give more accurate"
            " results for high percentiles."),
          this, defaults::max_messages_per_second)
    , max_messages_per_millisecond(
        desc("max-messages-per-millisecond").help(
            "Configure the per-millisecond messages rate histogram to expect"
            " no more than this number of messages per millisecond."
            "  Higher values consume more memory, but give more accurate"
            " results for high percentiles."),
          this, defaults::max_messages_per_millisecond)
    , max_messages_per_microsecond(
        desc("max-messages-per-microsecond").help(
            "Configure the per-microsecond messages rate histogram to expect"
            " no more than this number of messages per microsecond."
            "  Higher values consume more memory, but give more accurate"
            " results for high percentiles."),
          this, defaults::max_messages_per_microsecond)
    , max_interarrival_time_nanoseconds(
        desc("max-interarrival-time-nanoseconds").help(
            "Configure the interarrival time histogram to expect"
            " no more than this time between messages."
            "  Higher values consume more memory, but give more accurate"
            " results for high percentiles."),
          this, defaults::max_interarrival_time_nanoseconds)
    , max_processing_latency_nanoseconds(
        desc("max-processing-time-nanoseconds").help(
            "Configure the processing latency histogram to expect"
            " no more that no processing time is higher than this value."
            "   Higher values consume more memory, but give more accurate"
            " results for high percentiles."),
          this, defaults::max_processing_latency_nanoseconds)
    , reporting_interval_seconds(
        desc("reporting-interval-seconds").help(
            "Configure how often the statistics are logged."
            "  Use 0 to suppress all logging."
            "  The time is measured using the even timestamps,"
            " for feeds using recorded or simulated timestamps the"
            " reporting interval will not match the wall time."),
        this, defaults::reporting_interval_seconds)
{}

void jb::offline_feed_statistics::config::validate() const {
  if (max_messages_per_second() <= 1) {
    std::ostringstream os;
    os << "max-messages-per-second must be > 1, value="
       << max_messages_per_second(); 
    throw jb::usage(os.str(), 1);
  }

  if (max_messages_per_millisecond() <= 1) {
    std::ostringstream os;
    os << "max-messages-per-millisecond must be > 1, value="
       << max_messages_per_millisecond(); 
    throw jb::usage(os.str(), 1);
  }

  if (max_messages_per_microsecond() <= 1) {
    std::ostringstream os;
    os << "max-messages-per-microsecond must be > 1, value="
       << max_messages_per_microsecond(); 
    throw jb::usage(os.str(), 1);
  }

  if (max_interarrival_time_nanoseconds() <= 1) {
    std::ostringstream os;
    os << "max-interarrival-time-nanoseconds must be > 1, value="
       << max_interarrival_time_nanoseconds(); 
    throw jb::usage(os.str(), 1);
  }

  if (max_processing_latency_nanoseconds() <= 1) {
    std::ostringstream os;
    os << "max-processing_latency-nanoseconds must be > 1, value="
       << max_processing_latency_nanoseconds(); 
    throw jb::usage(os.str(), 1);
  }

  if (reporting_interval_seconds() < 0) {
    std::ostringstream os;
    os << "reporting-interval-seconds must be > 1, value="
       << reporting_interval_seconds(); 
    throw jb::usage(os.str(), 1);
  }
}
