#ifndef jb_book_cache_aware_stats_hpp
#define jb_book_cache_aware_stats_hpp

#include <jb/itch5/order_book_def.hpp>
#include <jb/config_object.hpp>
#include <jb/event_rate_histogram.hpp>
#include <jb/histogram.hpp>
#include <jb/integer_range_binning.hpp>

#include <iosfwd>

namespace jb {

typedef jb::itch5::tick_t tick_t;
typedef jb::itch5::level_t level_t;

/**
 * Keep statistics about changes on the inside
 * of a book, as well as tail movements
 */
class book_cache_aware_stats {
public:
  class config;

  /// Constructor
  explicit book_cache_aware_stats(config const& cfg);

  /**
   * Record a sample, that is book depth value after the event.
   *
   * @tparam book_depth_t the type used to record
   * the book depth after processing the event.
   * @param book_depth : the book depth (after processing the event)
   * to be recorded.
   */
  void sample(tick_t ticks, level_t levels) {
    ticks_.sample(ticks);
    levels_.sample(levels);
  }

  /**
   * Print a CSV header.
   *
   * The fields include:
   * - name: the name of the book_cache_aware_stats<> object.
   * - nsamples: the number of samples received.
   * - minTickInside: the minimum ticks observed
   * - p25TickInside: the 25th percentile for ticks observed
   * - p50TickInside: the 50th percentile for ticks observed
   * - p75TickInside: the 75th percentile for ticks observed
   * - p90TickInside: the 90th percentile for ticks observed
   * - p99TickInside: the 99th percentile for ticks observed
   * - p999TickInside: the 99.9th percentile for ticks observed
   * - p9999TickInside: the 99.99th percentile for ticks observed
   * - maxTickInside: the maximum for ticks observed
   * - minPriceLevels: the minimum price levels observed
   * - p25PriceLevels: the 25th percentile for price levels observed
   * - p50PriceLevels: the 50th percentile for price levels observed
   * - p75PriceLevels: the 75th percentile for price levels observed
   * - p90PriceLevels: the 90th percentile for price levels observed
   * - p99PriceLevels: the 99th percentile for price levels observed
   * - p999PriceLevels: the 99.9th percentile for price levels observed
   * - p9999PriceLevels: the 99.99th percentile for price levels observed
   * - maxPriceLevels: the maximum for price levels observed
   *
   * @param os the output stream
   */
  static void print_csv_header(std::ostream& os);

  /**
   * Print all the measurements in CSV format.
   */
  void print_csv(std::string const& name, std::ostream& os) const;

private:
  typedef histogram<integer_range_binning<tick_t>> tick_histogram_t;
  typedef histogram<integer_range_binning<level_t>> level_histogram_t;
  tick_histogram_t ticks_;
  level_histogram_t levels_;
};

/**
 * Configure a book_cache_aware_stats object.
 */
class book_cache_aware_stats::config : public jb::config_object {
public:
  config();
  config_object_constructors(config);

  /// Validate the configuration
  void validate() const override;

  /// No more than this value is recorded
  jb::config_attribute<config, tick_t> max_ticks;
  jb::config_attribute<config, level_t> max_levels;
};

} // namespace jb

#endif // jb_book_cache_aware_stats_hpp
