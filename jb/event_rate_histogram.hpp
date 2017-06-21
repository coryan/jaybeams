#ifndef jb_event_rate_histogram_hpp
#define jb_event_rate_histogram_hpp

#include <jb/event_rate_estimator.hpp>
#include <jb/histogram.hpp>
#include <jb/integer_range_binning.hpp>

namespace jb {

/**
 * Keep a histogram of observed event rates.
 *
 * This class collects running statistics about event rates.  While an
 * event_rate_estimator computes the number of events in the last X
 * seconds, this class keeps a tally of the observed event rates,
 * which can be used to build median, maximum and any other quantile
 * of the message rate.
 *
 * The user specifies the maximum message rate to keep at full
 * resolution, any rate above that value is simply recorded in the
 * histogram overflow bucket.
 *
 * For example to keep track of per-millisecond message rate you would
 * instantiate the class as follows:
 *
 * @code
 * typedef event_rate_histogram<
 *     int, std::chrono::microseconds> my_rate_histogram;
 * int max_expected_rate = 1000000;
 * my_rate_histogram rate_histo(
 *     max_expected_rate, std::chrono::microseconds(1000));
 *
 * ...
 * rate_histo.sample(timestamp);
 * ...
 * rate_histo.sample(timestamp);
 * @endcode
 *
 * The user of the class must provide a maximum expected message rate,
 * higher rates are recorded in a single 'overflow' bucket.  If you
 * guess too high on the rate the rate_histo object would consume more
 * memory than necessary.  If you guess too low the high quantiles
 * might be less accurate.  Memory requirements are low (the data
 * structure is basically a vector of ints), but can be an issue if
 * you create many histograms (for example, one histogram per security
 * when analyzing a market data feed).
 *
 * @tparam duration_type Define the units used to measure time.  This
 *   class assumes all events are timestamped with a class compatible
 *   with std::chrono::duration<>, against some epoch that is defined
 *   by convention but not explicitly provided to the class.  This is
 *   convenient because some feeds just provide microseconds or
 *   nanoseconds since midnight, while others use microseconds since
 *   the Unix Epoch.  If you are using some kind of
 *   std::chrono::time_point<> object for your timestamps simply call
 *   time_since_epoch() on the object before sampling.
 * @tparam counter_type The type used in the histogram counters.  For
 *   most applications @a int is adequate, but you may use a
 *   smaller type if (a) you are memory constrained, and (b) you do
 *   not expect high values in the counters.  You might also consider
 *   a wider integral type if you expect very high number of events.
 * @tparam rate_counter_type The type used in the event rate
 *   counters.  Similar tradeofss as @a counter_type.
 */
template <
    typename duration_type = std::chrono::microseconds,
    typename counter_type = int, typename rate_counter_type = int>
class event_rate_histogram
    : private histogram<integer_range_binning<std::uint64_t>, counter_type> {
public:
  //@{
  /**
   * @name Type traits.
   */
  typedef integer_range_binning<std::uint64_t> binning_strategy;
  typedef histogram<binning_strategy, counter_type> rate_histogram;
  //@}

  /**
   * Constructor.
   *
   * @param max_expected_rate The histogram is kept at full resolution
   *   up to this rate, any periods with more events are counted only
   *   in the overflow bin.  If this value is too low for the observed
   *   rates, the high quantiles may not be very accurate.
   * @param measurement_period over what period we measure event rates.
   * @param sampling_period how often do we measure event rates.
   */
  event_rate_histogram(
      std::uint64_t max_expected_rate, duration_type measurement_period,
      duration_type sampling_period = duration_type(1))
      : rate_histogram(binning_strategy(0, max_expected_rate))
      , rate_(measurement_period, sampling_period)
      , last_rate_(0) {
  }

  /// Record a new sample.
  void sample(duration_type ts) {
    rate_.sample(ts, [this](std::uint64_t rate, std::uint64_t repeats) {
      this->last_rate_ = rate;
      this->rate_histogram::weighted_sample(rate, repeats);
    });
  }

  /// Get the last sample, if any.
  std::uint64_t last_rate() const {
    if (nsamples() == 0) {
      throw std::invalid_argument("No sample recorded yet");
    }
    return last_rate_;
  }

  //@{
  /**
   * @name Histogram accessors.
   */
  using rate_histogram::nsamples;
  using rate_histogram::observed_min;
  using rate_histogram::observed_max;
  using rate_histogram::estimated_mean;
  using rate_histogram::estimated_quantile;
  using rate_histogram::overflow_count;
  using rate_histogram::underflow_count;
  //@}

private:
  event_rate_estimator<duration_type, rate_counter_type> rate_;
  std::uint64_t last_rate_;
};

} // namespace jb

#endif // jb_event_rate_histogram_hpp
