#ifndef jb_testing_create_triangle_timeseries_hpp
#define jb_testing_create_triangle_timeseries_hpp

namespace jb {
namespace testing {

/**
 * Create a simple timeseries where the values look like a triangle.
 */
template<typename timeseries>
void create_triangle_timeseries(int nsamples, timeseries& ts) {
  typedef typename timeseries::value_type value_type;
  ts.resize(nsamples);
  float p4 = nsamples / 4;
  for (int i = 0; i != nsamples / 2; ++i) {
    ts[i] = value_type((i - p4) / p4);
    ts[i + nsamples / 2] = value_type((p4 - i) / p4);
  }
}

} // namespace testing
} // namespace jb

#endif // jb_testing_create_triangle_timeseries_hpp
