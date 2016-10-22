#ifndef jb_book_depth_stats_hpp
#define jb_book_depth_stats_hpp

#include <jb/config_object.hpp>
#include <jb/event_rate_histogram.hpp>
#include <jb/histogram.hpp>
#include <jb/integer_range_binning.hpp>

#include <iosfwd>

namespace jb {

/* Ticket https://github.com/coryan/jaybeams/issues/20
 *
 * Remove all the histograms copied from offline_statistics.*
 * Create a new histogram to handle book depth samples
 * Set the memory limit to 10000
 *
 */
  
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
 *
 * Note: In order to get the book_depth_t here jb/itch5/order_book_depth.hpp
 * needs to be included. I decided to define book_depth_stats_t here to avoid that include
 */
typedef unsigned long int book_depth_stats_t;

class book_depth_statistics {
 public:
  class config;

  /// Constructor
  explicit book_depth_statistics(config const& cfg);

  /**
   * Record a sample, that is book depth after the event
   *
   * @tparam event_timestamp_t the type used to record the event
   * timestamps.
   * @tparam book_depth_t the type used to record the book depth after processing the event
   *
   * @param ts the event timestamp, please see the class documentation
   * for timestamps vs. time points.
   * @param book_depth : the book depth after processing the event.
   */
  template<typename event_timestamp_t, typename book_depth_stats_t>
  void sample(event_timestamp_t ts, const book_depth_stats_t& book_depth) {
    record_sample_book_depth(std::chrono::duration_cast<std::chrono::nanoseconds>(ts),
			     book_depth);
  }

  /**
   * Print a CSV header.
   *
   * The fields include:
   * - name: the name of the book_depth_statistics<> object.
   * - nsamples: the number of samples received.
   * - minBookDepth: the minimum book depth observed
   * - p25BookDepth: the 25th percentile for book depth observed
   * - p50BookDepth: the 50th percentile for book depth observed
   * - p75BookDepth: the 75th percentile for book depth observed
   * - p90BookDepth: the 90th percentile for book depth observed
   * - p99BookDepth: the 99th percentile for book depth observed
   * - p999BookDepth: the 99.9th percentile for book depth observed
   * - p9999BookDepth: the 99.99th percentile for book depth observed
   * - maxBookDepth: the maximum for book depth observed
   *
   * @param os the output stream
   */
  static void print_csv_header(std::ostream& os);

  /**
   * Print all the measurements in CSV format
   */
  void print_csv(std::string const& name, std::ostream& os) const;

 private:
  void record_sample_book_depth(std::chrono::nanoseconds ts, const book_depth_stats_t& book_depth);

 private:
  typedef histogram<integer_range_binning<book_depth_stats_t>> book_depth_histogram_t;
  book_depth_histogram_t book_depth_;  
};

/**
 * Configure a book_depth_statistics object
 */
class book_depth_statistics::config : public jb::config_object {
 public:
  config();
  config_object_constructors(config);

  /// Validate the configuration
  void validate() const override;

  // no more than this value is recorded 
  jb::config_attribute<config,book_depth_stats_t> max_book_depth;  
};

} // namespace jb

#endif // jb_book_depth_statistics_hpp
