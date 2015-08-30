#ifndef jb_testing_create_square_timeseries_hpp
#define jb_testing_create_square_timeseries_hpp

#include <jb/timeseries.hpp>
#include <chrono>

namespace jb {
namespace testing {

/**
 * Create a simple timeseries where the values look like a square.
 */
template<
  typename sample_t = float,
  typename duration_t = std::chrono::microseconds>
jb::timeseries<sample_t,duration_t> create_square_timeseries(int nsamples) {
  jb::timeseries<sample_t,duration_t> a(duration_t(1), duration_t(0), nsamples);
  float p4 = nsamples / 4;
  for (int i = 0; i != nsamples; ++i) {
    if (i < nsamples / 4) {
      a[i] = sample_t(-100);
    } else if (i < 3 * nsamples / 4) {
      a[i] = sample_t(+100);
    } else {
      a[i] = sample_t(-100);
    }
  }
  return std::move(a);
}

} // namespace testing
} // namespace jb

#endif // jb_testing_create_square_timeseries_hpp
