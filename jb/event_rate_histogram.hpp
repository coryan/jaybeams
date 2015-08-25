#ifndef jb_event_rate_histogram_hpp
#define jb_event_rate_histogram_hpp

#include <jb/histogram.hpp>
#include <jb/event_rate_estimator.hpp>
#include <jb/integer_range_binning.hpp>

namespace jb {

/**
 * Keep a histogram of observed event rates.
 *
 * This class composes a histogram and an event_rate_estimator to
 * collect running statistics about event rates.  While an
 * event_rate_estimator computes the number of events in the last X
 * seconds, this class keeps a tally of the observed event rates,
 * which can be used to build median, maximum and any other quantile
 * of the message rate.
 *
 * The user specifies the maximum message rate to keep at full
 * resolution, any rate above that value is simply recorded in the
 * histogram overflow bucket.
 *
 * @tparam counter_type The type used in the histogram counters.  For
 *   most applications @a int is adequate, but you may use a smaller
 *   type if (a) you are memory constrained, and (b) you do not expect
 *   high values in the counters.
 * @tparam duration_type Define the bucket size for the
 *   event_rate_estimator.
 */
template<
  typename counter_type = int,
  typename duration_type = std::chrono::microseconds>
class event_rate_histogram
    : private histogram<integer_range_binning<std::uint64_t>, counter_type> {
 public:
  typedef integer_range_binning<std::uint64_t> binning_strategy;
  typedef histogram<binning_strategy,counter_type> rate_histogram;

  /**
   * Constructor.
   *
   * @param max_expected_rate The histogram is kept at full resolution
   *   up to this rate, any periods with more events are counted only
   *   in the overflow bin.
   * @param period Count event event @a period microseconds.
   */
  event_rate_histogram(
      std::uint64_t max_expected_rate,
      duration_type period)
      : rate_histogram(binning_strategy(0, max_expected_rate))
      , rate_(period) {
  }

  /// Record a new sample.
  void sample(duration_type ts) {
    rate_.sample(ts, [this](std::uint64_t rate, std::uint64_t repeats) {
        this->last_rate_ = rate;
        this->rate_histogram::weighted_sample(rate, repeats); });
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
  event_rate_estimator<duration_type> rate_;
  std::uint64_t last_rate_;

};

} // namespace jb

#endif // jb_event_rate_histogram_hpp
