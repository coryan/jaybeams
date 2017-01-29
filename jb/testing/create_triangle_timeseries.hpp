#ifndef jb_testing_create_triangle_timeseries_hpp
#define jb_testing_create_triangle_timeseries_hpp

#include <jb/detail/array_traits.hpp>
#include <jb/fftw/aligned_multi_array.hpp>
#include <jb/testing/resize_if_applicable.hpp>
#include <boost/multi_array.hpp>

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
 * Create a family of timeseries where the values look like parallel triangles
 * shifted by a constant value.
 *
 * @param nsamples samples on each timeseries
 * @param ts multi array containing timeseries to be filled as triangle
 *
 * @tparam T type of the timeseries field
 * @tparam K timeseries dimensionality
 * @tparam A an Allocator type for type T allocator storage
 */
template <typename T, std::size_t K, typename A>
void create_triangle_timeseries(int nsamples, boost::multi_array<T, K, A>& ts) {
  /// The values stored in the input timeseries
  using value_type = T;
  assert(jb::detail::nsamples(ts) == static_cast<std::size_t>(nsamples));
  float p4 = nsamples / 4;
  int num_timeseries = jb::detail::element_count(ts) / nsamples;
  // fill the first timeseries
  int count = 0;
  for (int i = 0; i != nsamples / 2; ++i, ++count) {
    ts.data()[count] = value_type((i - p4) / p4);
    ts.data()[count + nsamples / 2] = value_type((p4 - i) / p4);
  }
  // ... the rest of the timeseries are shifted
  int shift = nsamples / num_timeseries;
  count = nsamples;
  for (int j = 1; j != num_timeseries; ++j) {
    for (int i = 0; i != nsamples; ++i, ++count) {
      ts.data()[count] = ts.data()[count + shift - nsamples];
    }
  }
}

} // namespace testing
} // namespace jb

#endif // jb_testing_create_triangle_timeseries_hpp
