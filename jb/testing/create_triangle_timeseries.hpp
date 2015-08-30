#ifndef jb_testing_create_triangle_timeseries_hpp
#define jb_testing_create_triangle_timeseries_hpp

#include <jb/timeseries.hpp>
#include <chrono>

namespace jb {
namespace testing {

/**
 * Create a simple timeseries where the values look like a triangle.
 */
template<
  typename sample_t = float,
  typename duration_t = std::chrono::microseconds>
jb::timeseries<sample_t,duration_t> create_triangle_timeseries(int nsamples) {
  jb::timeseries<sample_t,duration_t> a(duration_t(1), duration_t(0), nsamples);
  float p4 = nsamples / 4;
  for (int i = 0; i != nsamples / 2; ++i) {
    a[i] = sample_t((i - p4) / p4);
    a[i + nsamples / 2] = sample_t((p4 - i) / p4);
  }
  return std::move(a);
}

} // namespace testing
} // namespace jb

#endif // jb_testing_create_triangle_timeseries_hpp
