#ifndef jb_histogram_binning_liner_interpolation_hpp
#define jb_histogram_binning_liner_interpolation_hpp

#include <cmath>

namespace jb {

/**
 * Convenience function to implement the @ref binning_strategy_concept.
 *
 * The histogram class requires the binning strategy to solve the
 * follow equation for @f$x@f$:
 *
 * @f[
 *     {{(x - x_a)} \over {(y - y_a)}} = {{(x_b - x_a)} \over {s}}
 * @f]
 *
 * this is used in the computation of percentiles to better
 * approximate the percentile value between different cuts.  Some
 * binning strategies might provide a non-linear interpolation, but in
 * practice this (linear) solution is used in most cases.
 */
template<typename sample_type>
inline sample_type histogram_binning_linear_interpolation(
      sample_type x_a, sample_type x_b, double y_a, double s, double q) {
  return sample_type(std::floor(x_a + (q - y_a) * (x_b - x_a) / s));
}


} // namespace jb

#endif // jb_histogram_binning_linear_interpolation_hpp
