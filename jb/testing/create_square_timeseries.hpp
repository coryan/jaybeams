#ifndef jb_testing_create_square_timeseries_hpp
#define jb_testing_create_square_timeseries_hpp

namespace jb {
namespace testing {

/**
 * Create a simple timeseries where the values look like a square.
 */
template <typename timeseries>
void create_square_timeseries(int nsamples, timeseries& ts) {
  typedef typename timeseries::value_type value_type;
  ts.resize(nsamples);
  for (int i = 0; i != nsamples; ++i) {
    if (i < nsamples / 4) {
      ts[i] = value_type(-100);
    } else if (i < 3 * nsamples / 4) {
      ts[i] = value_type(+100);
    } else {
      ts[i] = value_type(-100);
    }
  }
}

} // namespace testing
} // namespace jb

#endif // jb_testing_create_square_timeseries_hpp
