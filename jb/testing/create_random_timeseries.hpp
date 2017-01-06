#ifndef jb_testing_create_random_timeseries_hpp
#define jb_testing_create_random_timeseries_hpp

#include <jb/testing/create_random_sample.hpp>

namespace jb {
namespace testing {

/**
 * Create a simple timeseries where the values look like a random.
 */
template <typename timeseries, typename generator>
void create_random_timeseries(generator& gen, int nsamples, timeseries& ts) {
  typedef typename timeseries::value_type value_type;
  ts.resize(nsamples);
  jb::testing::create_random_sample<value_type> create;
  for (int i = 0; i != nsamples; ++i) {
    ts[i] = create(gen);
  }
}

} // namespace testing
} // namespace jb

#endif // jb_testing_create_random_timeseries_hpp
