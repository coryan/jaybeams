#ifndef jb_offline_feed_stats_hpp
#define jb_offline_feed_stats_hpp

#include <jb/config_object.hpp>
#include <jb/event_rate_histogram.hpp>
#include <jb/histogram.hpp>
#include <jb/integer_range_binning.hpp>

#include <iosfwd>

namespace jb {

/**
 * Keep statistics about a feed and its offline processor.
 *
 * In JayBeams we will find ourselves processing all kinds of offline
 * feeds, that is, files with the contents of a real-time feed.  We
 * want to measure things like the processing time for the data, what
 * is the distribution of message rates like, what is the distribution
 * of mesage inter-arrival times like.
 *
 * This class encapsulates a lot of the logic for those computations.
 * The class can be thoroughly configured at run-time, for ease of
 * experimentation, but the default values should be reasonable.
 *
 * The source of timestamps can be an actual clock, the recorded
 * timestamps is a feed file, or some virtual clock used in
 * simulations.  The only requirement is for the timestamps to be
 * compatible with std::chrono::duration<>.  It might seem odd to use
 * std::chrono::duration<> instead of std::chrono::time_point<>, this
 * is because most feeds are timestamps to a well known time point:
 * midnight at the beginning of the day, or 00:00:00am.
 *
 * Likewise, the processing latency measurements can be generated from
 * an actual clock or some virtual measurement (say counting
 * interrupts, or CPU cycles vs. elapsed time).  The only requirement
 * is for the measurements to be compatible with
 * std::chono::duration<>.
 */
class offline_feed_statistics {
public:
  class config;

  /// Constructor
  explicit offline_feed_statistics(config const& cfg);

  /**
   * Record a sample, that is process a message received at the given
   * timestamp.
   *
   * @tparam event_timestamp_t the type used to record the event
   * timestamps.
   * @tparam duration_t the type used to record the processing latency
   * for the event.
   *
   * @param ts the event timestamp, please see the class documentation
   * for timestamps vs. time points.
   * @param processing_latency the time it took to process the event.
   */
  template <typename event_timestamp_t, typename duration_t>
  void sample(event_timestamp_t ts, duration_t processing_latency) {
    using namespace std::chrono;
    record_sample(
        duration_cast<nanoseconds>(ts),
        duration_cast<nanoseconds>(processing_latency));
  }

  /**
   * Print a CSV header.
   *
   * Assuming there are many offline_feed_statistics<> objects (say
   * one for each symbol, or one for each minute interval) this
   * function can be used to print the CSV header for all of them.
   *
   * The fields include:
   * - name: the name of the offline_feed_statistics<> object.
   * - nsamples: the number of samples received.
   * - minRatePerSec: the minimum messages/second rate
   * - p25RatePerSec: the 25th percentile for the messages/second rate
   * - p50RatePerSec: the 50th percentile for the messages/second rate
   * - p75RatePerSec: the 75th percentile for the messages/second rate
   * - p90RatePerSec: the 90th percentile for the messages/second rate
   * - p99RatePerSec: the 99th percentile for the messages/second rate
   * - p999RatePerSec: the 99.9th percentile for the messages/second rate
   * - p9999RatePerSec: the 99.99th percentile for the messages/second rate
   * - maxRatePerSec: the maximum for the messages/second rate
   * - minRatePerMSec, p25RatePerMSec, ..., maxRatePerMSec: the
   *   statistics for the messages/millisecond rate.
   * - minRatePerUSec, p25RatePerUSec, ..., maxRatePerUSec: the
   *   statistics for the messages/microsecond rate.
   * - minProcessingLatency, ..., maxProcessingLatency: the statistics
   *   for processing latency of the events, in nanoseconds.
   * - minArrival: the minimum of the timestamp difference between two
   *   consecutive messages, in nanoseconds.
   * - p0001Arrival: the 0.01th percentile of the timestamp difference
   *   between two consecutive messages, in nanoseconds.
   * - p001Arrival: the 0.1th percentile of the timestamp difference
   *   between two consecutive messages, in nanoseconds.
   * - p01Arrival: the 1st percentile of the timestamp difference
   *   between two consecutive messages, in nanoseconds.
   * - p10Arrival: the 10th percentile of the timestamp difference
   *   between two consecutive messages, in nanoseconds.
   * - p25Arrival, p50Arrival, p75Arrival, maxArrival: more statistics
   *   about the arrival time.
   *
   * @param os the output stream
   */
  static void print_csv_header(std::ostream& os);

  /**
   * Print all the measurements in CSV format.
   */
  void print_csv(std::string const& name, std::ostream& os) const;

private:
  void record_sample(
      std::chrono::nanoseconds ts, std::chrono::nanoseconds processing_latency);

private:
  typedef event_rate_histogram<std::chrono::nanoseconds, std::int64_t>
      rate_histogram;
  rate_histogram per_sec_rate_;
  rate_histogram per_msec_rate_;
  rate_histogram per_usec_rate_;
  typedef histogram<integer_range_binning<std::int64_t>>
      interarrival_histogram_t;
  interarrival_histogram_t interarrival_;

  typedef histogram<integer_range_binning<std::uint64_t>>
      processing_latency_histogram_t;
  processing_latency_histogram_t processing_latency_;

  std::chrono::seconds reporting_interval_;
  std::chrono::nanoseconds last_ts_;
  std::chrono::nanoseconds last_report_ts_;
};

/**
 * Configure an offline_feed_statistics object
 */
class offline_feed_statistics::config : public jb::config_object {
public:
  config();
  config_object_constructors(config);

  /// Validate the configuration
  void validate() const override;

  jb::config_attribute<config, int> max_messages_per_second;
  jb::config_attribute<config, int> max_messages_per_millisecond;
  jb::config_attribute<config, int> max_messages_per_microsecond;
  jb::config_attribute<config, std::int64_t> max_interarrival_time_nanoseconds;
  jb::config_attribute<config, int> max_processing_latency_nanoseconds;
  jb::config_attribute<config, int> reporting_interval_seconds;
};

} // namespace jb

#endif // jb_offline_feed_statistics_hpp
