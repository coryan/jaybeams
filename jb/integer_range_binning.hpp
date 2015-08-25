#ifndef jb_integer_range_binning_hpp
#define jb_integer_range_binning_hpp

#include <cmath>
#include <limits>
#include <type_traits>

namespace jb {

/**
 * A histogram binning_strategy for integer numbers in a known range.
 *
 * This class defines histogram bins for samples with integer values
 * in a range defined at run-time.  Care must be taken if the range is
 * too big because the corresponding histogram is likely to consume a
 * lot of memory.   See jb::binning_strategy_concept.
 *
 * @tparam sample_type_t the type of samples, should be an integer type.
 */
template<typename sample_type_t>
class integer_range_binning {
 public:
  /// type traits
  typedef sample_type_t sample_type;

  /**
   * Constructor based on the histogram range.
   *
   * @param h_min The value for histogram_min()
   * @param h_max The value for histogram_max()
   */
  integer_range_binning(sample_type h_min, sample_type h_max)
      : h_min_(h_min)
      , h_max_(h_max) {
    static_assert(
        std::is_integral<sample_type>::value,
        "The sample_type must be an integral type");
  }

  sample_type histogram_min() const {
    return h_min_;
  }
  sample_type histogram_max() const {
    return h_max_;
  }
  sample_type theoretical_min() const {
    return std::numeric_limits<sample_type>::min();
  }
  sample_type theoretical_max() const {
    return std::numeric_limits<sample_type>::max();
  }
  std::size_t sample2bin(sample_type t) const {
    return static_cast<std::size_t>(t - histogram_min());
  }
  sample_type bin2sample(std::size_t i) const {
    return histogram_min() + i;
  }
  sample_type interpolate(
      sample_type x_a, sample_type x_b, double y_a, double s, double q) const {
    return sample_type(std::floor(x_a + (q - y_a) * (x_b - x_a) / s));
  }

 private:
  sample_type h_min_;
  sample_type h_max_;
};

} // namespace jb

#endif // jb_integer_rate_histogram_hpp
