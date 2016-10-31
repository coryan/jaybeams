#include "jb/offline_feed_statistics.hpp"

#include <jb/as_hhmmss.hpp>
#include <jb/log.hpp>

#include <iostream>

namespace {

template <typename event_rate_histogram_t>
void report_rate(
    std::chrono::nanoseconds ts, char const* period_name,
    event_rate_histogram_t const& histo) {
  JB_LOG(info) << "events/" << period_name << ": " << jb::as_hhmmss(ts)
               << ", min=" << histo.observed_min()
               << ", p25=" << histo.estimated_quantile(0.25)
               << ", p50=" << histo.estimated_quantile(0.50)
               << ", p75=" << histo.estimated_quantile(0.75)
               << ", p90=" << histo.estimated_quantile(0.90)
               << ", p99=" << histo.estimated_quantile(0.99)
               << ", p99.9=" << histo.estimated_quantile(0.999)
               << ", p99.99=" << histo.estimated_quantile(0.9999)
               << ", max=" << histo.observed_max()
               << ", N=" << histo.nsamples();
}

template <typename event_rate_histogram_t>
void csv_rate(std::ostream& os, event_rate_histogram_t const& histo) {
  os << "," << histo.observed_min() << "," << histo.estimated_quantile(0.25)
     << "," << histo.estimated_quantile(0.50) << ","
     << histo.estimated_quantile(0.75) << "," << histo.estimated_quantile(0.90)
     << "," << histo.estimated_quantile(0.99) << ","
     << histo.estimated_quantile(0.999) << ","
     << histo.estimated_quantile(0.9999) << "," << histo.observed_max();
}

template <typename latency_histogram_t>
void report_arrival(
    std::chrono::nanoseconds ts, char const* name,
    latency_histogram_t const& histo) {
  JB_LOG(info) << name << ": " << jb::as_hhmmss(ts)
               << ", min=" << histo.observed_min()
               << "ns, p0.01=" << histo.estimated_quantile(0.0001)
               << "ns, p0.1=" << histo.estimated_quantile(0.001)
               << "ns, p01=" << histo.estimated_quantile(0.01)
               << "ns, p05=" << histo.estimated_quantile(0.05)
               << "ns, p10=" << histo.estimated_quantile(0.10)
               << "ns, p25=" << histo.estimated_quantile(0.25)
               << "ns, p50=" << histo.estimated_quantile(0.50)
               << "ns, p75=" << histo.estimated_quantile(0.75)
               << "ns, p90=" << histo.estimated_quantile(0.90)
               << "ns, p99=" << histo.estimated_quantile(0.99)
               << "ns, max=" << histo.observed_max()
               << "ns, N=" << histo.nsamples();
}

template <typename latency_histogram_t>
void csv_arrival(std::ostream& os, latency_histogram_t const& histo) {
  os << "," << histo.observed_min() << "," << histo.estimated_quantile(0.0001)
     << "," << histo.estimated_quantile(0.001) << ","
     << histo.estimated_quantile(0.01) << "," << histo.estimated_quantile(0.05)
     << "," << histo.estimated_quantile(0.10) << ","
     << histo.estimated_quantile(0.25) << "," << histo.estimated_quantile(0.50)
     << "," << histo.estimated_quantile(0.75) << ","
     << histo.estimated_quantile(0.90) << "," << histo.estimated_quantile(0.99)
     << "," << histo.observed_max();
}

template <typename latency_histogram_t>
void report_latency(
    std::chrono::nanoseconds ts, char const* name,
    latency_histogram_t const& histo) {
  JB_LOG(info) << name << ": " << jb::as_hhmmss(ts)
               << ", min=" << histo.observed_min()
               << "ns, p10=" << histo.estimated_quantile(0.10)
               << "ns, p25=" << histo.estimated_quantile(0.25)
               << "ns, p50=" << histo.estimated_quantile(0.50)
               << "ns, p75=" << histo.estimated_quantile(0.75)
               << "ns, p90=" << histo.estimated_quantile(0.90)
               << "ns, p99=" << histo.estimated_quantile(0.99)
               << "ns, p99.9=" << histo.estimated_quantile(0.999)
               << "ns, p99.99=" << histo.estimated_quantile(0.9999)
               << "ns, max=" << histo.observed_max()
               << "ns, N=" << histo.nsamples();
}

template <typename latency_histogram_t>
void csv_latency(std::ostream& os, latency_histogram_t const& histo) {
  os << "," << histo.observed_min() << "," << histo.estimated_quantile(0.10)
     << "," << histo.estimated_quantile(0.25) << ","
     << histo.estimated_quantile(0.50) << "," << histo.estimated_quantile(0.75)
     << "," << histo.estimated_quantile(0.90) << ","
     << histo.estimated_quantile(0.99) << "," << histo.estimated_quantile(0.999)
     << "," << histo.estimated_quantile(0.9999) << "," << histo.observed_max();
}

} // anonymous namespace

jb::offline_feed_statistics::offline_feed_statistics(config const& cfg)
    : per_sec_rate_(
          cfg.max_messages_per_second(), std::chrono::seconds(1),
          std::chrono::milliseconds(1))
    , per_msec_rate_(
          cfg.max_messages_per_millisecond(), std::chrono::milliseconds(1),
          std::chrono::microseconds(1))
    , per_usec_rate_(
          cfg.max_messages_per_microsecond(), std::chrono::microseconds(1),
          std::chrono::nanoseconds(1))
    , interarrival_(
          interarrival_histogram_t::binning_strategy(
              0, cfg.max_interarrival_time_nanoseconds()))
    , processing_latency_(
          processing_latency_histogram_t::binning_strategy(
              0, cfg.max_processing_latency_nanoseconds()))
    , reporting_interval_(
          std::chrono::seconds(cfg.reporting_interval_seconds()))
    , last_ts_(0) {
}

void jb::offline_feed_statistics::print_csv_header(std::ostream& os) {
  char const* fields[] = {"min", "p25",  "p50",   "p75", "p90",
                          "p99", "p999", "p9999", "max"};
  char const* tracked[] = {"RatePerSec", "RatePerMSec", "RatePerUSec"};
  os << "Name,NSamples";
  for (auto t : tracked) {
    for (auto f : fields) {
      os << "," << f << t;
    }
  }
  os << ",minArrival,p0001Arrival,p001Arrival,p01Arrival"
     << ",p05Arrival,p10Arrival,p25Arrival,p50Arrival,p75Arrival"
     << ",p90Arrival,p99Arrival,maxArrival";
  os << ",minProcessing,p10Processing,p25Processing,p50Processing"
     << ",p75Processing,p90Processing,p99Processing,p999Processing"
     << ",p9999Processing,maxProcessing";
  os << "\n";
}

void jb::offline_feed_statistics::record_sample(
    std::chrono::nanoseconds ts, std::chrono::nanoseconds pl) {
  using namespace std::chrono;
  per_sec_rate_.sample(ts);
  per_msec_rate_.sample(ts);
  per_usec_rate_.sample(ts);

  if (processing_latency_.nsamples() > 0) {
    nanoseconds d = ts - last_ts_;
    interarrival_.sample(d.count());
  } else {
    last_report_ts_ = ts;
  }
  processing_latency_.sample(pl.count());
  last_ts_ = ts;

  if (ts - last_report_ts_ > reporting_interval_ and
      interarrival_.nsamples() > 0) {
    report_rate(ts, "sec ", per_sec_rate_);
    report_rate(ts, "msec", per_msec_rate_);
    report_rate(ts, "usec", per_usec_rate_);
    report_arrival(ts, "arrival    ", interarrival_);
    report_latency(ts, "processing ", processing_latency_);
    last_report_ts_ = ts;
  }
}

void jb::offline_feed_statistics::print_csv(
    std::string const& name, std::ostream& os) const {
  if (per_sec_rate_.nsamples() == 0 or per_msec_rate_.nsamples() == 0 or
      per_usec_rate_.nsamples() == 0 or interarrival_.nsamples() == 0 or
      processing_latency_.nsamples() == 0) {
    os << name << ",0";
    os << ",,,,,,,,,";    // per-second rate
    os << ",,,,,,,,,";    // per-millisecond rate
    os << ",,,,,,,,,";    // per-microsecond rate
    os << ",,,,,,,,,,,,"; // interarrival
    os << ",,,,,,,,,,";   // processing latency
    os << "\n";
    return;
  }
  os << name << "," << processing_latency_.nsamples();
  csv_rate(os, per_sec_rate_);
  csv_rate(os, per_msec_rate_);
  csv_rate(os, per_usec_rate_);
  csv_arrival(os, interarrival_);
  csv_latency(os, processing_latency_);
  os << std::endl;
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
int reporting_interval_seconds = JB_OFS_DEFAULTS_reporting_interval_seconds;

} // namespace defaults
} // namespace jb

jb::offline_feed_statistics::config::config()
    : max_messages_per_second(
          desc("max-messages-per-second")
              .help(
                  "Configure the per-second messages rate histogram to expect"
                  " no more than this number of messages per second."
                  "  Higher values consume more memory, but give more accurate"
                  " results for high percentiles."),
          this, defaults::max_messages_per_second)
    , max_messages_per_millisecond(
          desc("max-messages-per-millisecond")
              .help(
                  "Configure the per-millisecond messages rate histogram to "
                  "expect"
                  " no more than this number of messages per millisecond."
                  "  Higher values consume more memory, but give more accurate"
                  " results for high percentiles."),
          this, defaults::max_messages_per_millisecond)
    , max_messages_per_microsecond(
          desc("max-messages-per-microsecond")
              .help(
                  "Configure the per-microsecond messages rate histogram to "
                  "expect"
                  " no more than this number of messages per microsecond."
                  "  Higher values consume more memory, but give more accurate"
                  " results for high percentiles."),
          this, defaults::max_messages_per_microsecond)
    , max_interarrival_time_nanoseconds(
          desc("max-interarrival-time-nanoseconds")
              .help(
                  "Configure the interarrival time histogram to expect"
                  " no more than this time between messages."
                  "  Higher values consume more memory, but give more accurate"
                  " results for high percentiles."),
          this, defaults::max_interarrival_time_nanoseconds)
    , max_processing_latency_nanoseconds(
          desc("max-processing-time-nanoseconds")
              .help(
                  "Configure the processing latency histogram to expect"
                  " no more that no processing time is higher than this value."
                  "   Higher values consume more memory, but give more accurate"
                  " results for high percentiles."),
          this, defaults::max_processing_latency_nanoseconds)
    , reporting_interval_seconds(
          desc("reporting-interval-seconds")
              .help(
                  "Configure how often the statistics are logged."
                  "  Use 0 to suppress all logging."
                  "  The time is measured using the even timestamps,"
                  " for feeds using recorded or simulated timestamps the"
                  " reporting interval will not match the wall time."),
          this, defaults::reporting_interval_seconds) {
}

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
