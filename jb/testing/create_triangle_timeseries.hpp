#ifndef jb_testing_create_triangle_timeseries_hpp
#define jb_testing_create_triangle_timeseries_hpp

#include <jb/detail/array_traits.hpp>
#include <jb/fftw/aligned_multi_array.hpp>
#include <jb/fftw/alignment_traits.hpp>
#include <jb/testing/resize_if_applicable.hpp>

namespace jb {
namespace testing {

/**
 * Create a simple timeseries where the values look like a triangle.
 */
template <typename timeseries>
void create_triangle_timeseries(int nsamples, timeseries& ts) {
  resize_if_applicable(ts, nsamples);
  using value_type = typename timeseries::value_type;
  float p4 = nsamples / 4;
  for (int i = 0; i != nsamples / 2; ++i) {
    ts[i] = value_type((i - p4) / p4);
    ts[i + nsamples / 2] = value_type((p4 - i) / p4);
  }
}

/**
 * Create timeseries where the values look like a triangle
 * @param nsamples samples on each timeseries
 * @param ts multi array containing timeseries to be filled as triangle
 *
 * @tparam T type of the timeseries field
 * @tparam K timeseries dimensionality
 */

template <typename T, std::size_t K>
void create_triangle_timeseries(
    int nsamples, jb::fftw::aligned_multi_array<T, K>& ts) {
  /// The values stored in the input timeseries
  using value_type = T;

  assert(jb::detail::nsamples(ts) == static_cast<std::size_t>(nsamples));
  float p4 = nsamples / 4;
  int num_timeseries = jb::detail::element_count(ts) / nsamples;
  int count = 0;
  for (int j = 0; j != num_timeseries; ++j) {
    for (int i = 0; i != nsamples / 2; ++i) {
      ts.data()[count] = value_type((i - p4) / p4);
      ts.data()[count + nsamples / 2] = value_type((p4 - i) / p4);
      count++;
    }
    count += nsamples / 2;
  }
}

} // namespace testing
} // namespace jb

#endif // jb_testing_create_triangle_timeseries_hpp
