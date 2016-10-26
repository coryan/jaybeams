#ifndef jb_book_depth_stats_hpp
#define jb_book_depth_stats_hpp

#include <jb/config_object.hpp>
#include <jb/event_rate_histogram.hpp>
#include <jb/histogram.hpp>
#include <jb/integer_range_binning.hpp>

#include <iosfwd>

namespace jb {

/**
 * Keep statistics about a feed and its book depth.
 *
 */
typedef unsigned long int book_depth_stats_t;

class book_depth_statistics {
 public:
  class config;

  /// Constructor
  explicit book_depth_statistics(config const& cfg);

  /**
   * Record a sample, that is book depth value after the event.
   *
   * @tparam event_timestamp_t the type used to record the event
   * timestamps.
   * @tparam book_depth_t the type used to record the book depth after processing the event
   *
   * @param ts the event timestamp, please see the class documentation
   * for timestamps vs. time points.
   * @param book_depth : the book depth (after processing the event) to be recorded.
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
   * Print all the measurements in CSV format.
   */
  void print_csv(std::string const& name, std::ostream& os) const;

 private:
  void record_sample_book_depth(std::chrono::nanoseconds ts, const book_depth_stats_t& book_depth);

 private:
  typedef histogram<integer_range_binning<book_depth_stats_t>> book_depth_histogram_t;
  book_depth_histogram_t book_depth_;  
};

/**
 * Configure a book_depth_statistics object.
 */
class book_depth_statistics::config : public jb::config_object {
 public:
  config();
  config_object_constructors(config);

  /// Validate the configuration
  void validate() const override;

  /// No more than this value is recorded 
  jb::config_attribute<config,book_depth_stats_t> max_book_depth;  
};

} // namespace jb

#endif // jb_book_depth_statistics_hpp
