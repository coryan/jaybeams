#ifndef jb_histogram_hpp
#define jb_histogram_hpp

#include <jb/histogram_summary.hpp>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace jb {

/**
 * A histogram class with controllable binning and range strategy.
 *
 * We are interested in capturing histograms of latency, and of rate
 * measurements, and other things.  The challenge is that we need good
 * precision, but we also need to keep thousands of these histograms
 * in memory.  To increase precision in histograms one must increase
 * the number of bins, but that increases the memory requirements.
 *
 * Consider, for example, a typical latency measurement that typically
 * ranges from 1 to 400 microseconds, but sometimes can peak to
 * seconds.  Capturing the full range at microsecond precision would
 * require MiB of memory.  While this is not a problem for a single
 * histogram, what if we want to keep this histogram per symbol?
 *
 * As an alternative, we can keep this histogram at full resolution
 * between 0 and 1,000 microseconds, drop to 10 microsecond bins from
 * there to 10,000 microseconds, then increase to 100 microsecond bins
 * and so forth.  For values higher than a prescribed limit we simply
 * record "overflow".
 *
 * This class implements such a histogram, with a user-provided
 * mapping strategy to define the bins.
 *
 * @tparam binning_strategy_t Please see @ref jb::binning_strategy_concept.
 * @tparam counter_type The type used for the counters.  In most
 *   applications using int is sufficient, but if you are counting
 *   really large numbers you might consider using std::uint64_t, or
 *   if you are memory constrained consider using std::int16_t.
 */
template<typename binning_strategy_t, typename counter_type_t = unsigned int>
class histogram {
  struct check_constraints;
 public:
  //@{
  /**
   * @name type traits
   */
  typedef binning_strategy_t binning_strategy;
  typedef typename binning_strategy::sample_type sample_type;
  typedef counter_type_t counter_type;
  //@}
  
  /**
   * Construct a histogram given a mapping strategy.
   *
   * Most mapping strategies are likely to be stateless, so the
   * default constructor is adequate.
   *
   * @param mapping Define how samples are mapped into bins.
   */
  explicit histogram(binning_strategy const& mapping = binning_strategy())
      : binning_(mapping)
      , underflow_count_(0)
      , overflow_count_(0)
      , observed_min_(binning_.theoretical_max())
      , observed_max_(binning_.theoretical_min())
      , nsamples_(0)
      , bins_(nbins()) {
    check_constraints checker;
  }

  /// Return the number of samples observed to this point.
  std::uint64_t nsamples() const {
    return nsamples_;
  }

  /// Return the smallest sample value observed to this point.
  sample_type observed_min() const {
    return observed_min_;
  }

  /// Return the largest sample value observed to this point.
  sample_type observed_max() const {
    return observed_max_;
  }

  /**
   * Estimate the mean of the sample distribution.
   *
   * Estimating the mean is O(N) where N is the number of bins.
   */
  sample_type estimated_mean() const {
    if (nsamples_ == 0) {
      throw std::invalid_argument(
          "Cannot estimate mean on an empty histogram");
    }
    sample_type acc = sample_type();
    // ... use the midpoint of the underflow bin to estimate the
    // contribution of those samples ...
    if (underflow_count()) {
      acc += (midpoint(observed_min(), binning_.histogram_min())
              * underflow_count());
    }
    sample_type a = binning_.histogram_min();
    for(std::size_t i = 0; i != bins_.size(); ++i) {
      sample_type b = binning_.bin2sample(i + 1);
      acc += midpoint(a, b) * bins_[i];
      a = b;
    }
    if (overflow_count()) {
      acc += (midpoint(binning_.histogram_max(), observed_max())
              * overflow_count());
    }
    return acc / nsamples_;
  }

  /**
   * Estimate a quantile of the sample distribution.
   *
   * The inverse of the accumulated density function.  Find the
   * smallest value Q such that at most q * nsamples of the samples
   * are smaller than Q.  If you prefer to think in percentiles make
   * @code
   *    q = percentile / 100.0
   * @endcode
   *
   * Estimating quantiles is O(N) where N is the number of bins.
   */
  sample_type estimated_quantile(double q) const {
    if (nsamples_ == 0) {
      throw std::invalid_argument(
          "Cannot estimate quantile for empty histogram");
    }
    if (q < 0 or q > 1.0) {
      throw std::invalid_argument(
          "Quantile value outside 0 <= q <= 1 range");
    }
    std::uint64_t cum_samples = 0;
    std::uint64_t bin_samples = underflow_count();
    if (bin_samples and q <= double(cum_samples + bin_samples) / nsamples()) {
      double s = double(bin_samples) / nsamples();
      double y_a = double(cum_samples) / nsamples();
      return binning_.interpolate(
          observed_min(), binning_.histogram_min(), y_a, s, q);
    }
    for(std::size_t i = 0; i != bins_.size(); ++i) {
      cum_samples += bin_samples;
      bin_samples = bins_[i];
      if (bin_samples and q <= double(cum_samples + bin_samples) / nsamples()) {
        double s = double(bin_samples) / nsamples();
        double y_a = double(cum_samples) / nsamples();
        return binning_.interpolate(
            binning_.bin2sample(i), binning_.bin2sample(i + 1), y_a, s, q);
      }
    }
    cum_samples += bin_samples;
    bin_samples = overflow_count();
    if (bin_samples and q <= double(cum_samples + bin_samples) / nsamples()) {
      double s = double(bin_samples) / nsamples();
      double y_a = double(cum_samples) / nsamples();
      return binning_.interpolate(
          binning_.histogram_max(), observed_max(), y_a, s, q);
    }

    return observed_max();
  }

  /// Return the number of samples smaller than the histogram range.
  std::uint64_t underflow_count() const {
    return underflow_count_;
  }

  /// Return the number of samples larger than the histogram range.
  std::uint64_t overflow_count() const {
    return overflow_count_;
  }

  /// Return a simple summary
  histogram_summary summary() const {
    if (nsamples() == 0) {
      return histogram_summary{0, 0, 0, 0, 0, 0, 0, 0};
    }
    return histogram_summary{
      double(observed_min())
          , double(estimated_quantile(0.25))
          , double(estimated_quantile(0.50))
          , double(estimated_quantile(0.75))
          , double(estimated_quantile(0.90))
          , double(estimated_quantile(0.99))
          , double(observed_max())
          , nsamples()
          };
  }

  /// Record a new sample.
  void sample(sample_type const& t) {
    weighted_sample(t, 1);
  }

  /// Record a new sample.
  void weighted_sample(sample_type const& t, counter_type weight) {
    if (weight == 0) {
      return;
    }
    nsamples_ += weight;
    if (observed_min_ > t) {
      observed_min_ = t;
    }
    if (observed_max_ < t) {
      observed_max_ = t;
    }
    if (binning_.histogram_min() <= t and t < binning_.histogram_max()) {
      auto i = binning_.sample2bin(t);
      bins_[i] += weight;
    } else if (t < binning_.histogram_min()) {
      underflow_count_ += weight;
    } else {
      overflow_count_ += weight;
    }
  }

  /// Reset all counters
  void reset() {
    histogram fresh(binning_);
    *this = std::move(fresh);
  }

  /// The type used to store the bins.
  typedef std::vector<counter_type> counters;

 private:
  /// Compute the maximum number of bins that might be needed.
  std::size_t nbins() const {
    std::size_t max = binning_.sample2bin(binning_.histogram_max());
    std::size_t min = binning_.sample2bin(binning_.histogram_min());
    return max - min;
  }

  /// Estimate a midpoint
  sample_type midpoint(sample_type const& a, sample_type const& b) const {
    return (a + b)/2;
  }

 private:
  binning_strategy binning_;
  std::uint64_t underflow_count_;
  std::uint64_t overflow_count_;
  sample_type observed_min_;
  sample_type observed_max_;
  std::uint64_t nsamples_;
  counters bins_;
};

/**
 * Verify the constraints on the histogram template class template
 * parameters, and generate better error messages when they are not
 * met.
 */
template<typename binning_strategy, typename counter_type>
struct histogram<binning_strategy,counter_type>::check_constraints {
  typedef histogram<binning_strategy> histo;
  typedef typename histo::sample_type sample_type;

  check_constraints() {
    static_assert(
        std::is_integral<counter_type>::value,
        "The counter_type must be an integral type");

    static_assert(
        std::is_convertible<decltype(
            histogram_min_return_type()), sample_type>::value,
        "The binning_strategy must provide a min() function, "
        "and it must return a type compatible with sample_type.");
    static_assert(
        std::is_convertible<decltype(
            histogram_max_return_type()), sample_type>::value,
        "The binning_strategy must provide a max() function, "
        "and it must return a type compatible with sample_type.");

    static_assert(
        std::is_convertible<decltype(
            theoretical_min_return_type()), sample_type>::value,
        "The binning_strategy must provide a theoretical_min() function, "
        "and it must return a type compatible with sample_type.");
    static_assert(
        std::is_convertible<decltype(
            theoretical_max_return_type()), sample_type>::value,
        "The binning_strategy must provide a theoretical_max() function, "
        "and it must return a type compatible with sample_type.");

    static_assert(
        std::is_convertible<decltype(
            interpolate_return_type()), sample_type>::value,
        "The binning_strategy must provide a interpolate() function, "
        "and it must return a type compatible with sample_type.");

    static_assert(
        std::is_convertible<
            decltype(sample2bin_return_type()), std::size_t>::value,
        "The binning_strategy must provide a sample2bin() function, "
        "and it must return a type compatible with std::size_t.");
    static_assert(
        std::is_convertible<
            decltype(bin2sample_return_type()), sample_type>::value,
        "The binning_strategy must provide a bin2sample() function, "
        "and it must return a type compatible with sample_type.");

    static_assert(
        1 == sizeof(decltype(has_less_than(std::declval<sample_type>()))),
        "The sample_type must have a < operator.");
    static_assert(
        1 == sizeof(decltype(has_less_than_or_equal(
            std::declval<sample_type>()))),
        "The sample_type must have a <= operator.");
  }

  auto histogram_min_return_type()
      -> decltype(std::declval<const binning_strategy>().histogram_min());
  auto histogram_max_return_type()
      -> decltype(std::declval<const binning_strategy>().histogram_max());

  auto theoretical_min_return_type()
      -> decltype(std::declval<const binning_strategy>().theoretical_min());
  auto theoretical_max_return_type()
      -> decltype(std::declval<const binning_strategy>().theoretical_max());
  
  auto sample2bin_return_type()
      -> decltype(std::declval<const binning_strategy>().sample2bin(
          std::declval<const sample_type>()));
  auto bin2sample_return_type()
      -> decltype(std::declval<const binning_strategy>().bin2sample(
          std::size_t(0)));

  auto interpolate_return_type()
      -> decltype(std::declval<const binning_strategy>().interpolate(
          std::declval<const sample_type>(), std::declval<const sample_type>(),
          double(1.0), double(1.0), double(0.5)));

  /// An object to create a SFINAE condition.
  struct error { char fill[2]; };

  // Use SFINAE to discover if the variable type can be compared as we
  // need to.
  error has_less_than(...);
  auto has_less_than(sample_type const& t)
      -> decltype(static_cast<void>(t < t), char(0));
  error has_less_than_or_equal(...);
  auto has_less_than_or_equal(sample_type const& t)
      -> decltype(static_cast<void>(t <= t), char(0));
};

} // namespace jb

#endif // jb_histogram_hpp

