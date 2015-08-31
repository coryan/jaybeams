#ifndef jb_testing_delay_timeseries_hpp
#define jb_testing_delay_timeseries_hpp

#include <jb/timeseries.hpp>
#include <chrono>

namespace jb {
namespace testing {

/**
 * Delay a timeseries using a periodic extension for early values.
 */
template<typename timeseries_t>
timeseries_t delay_timeseries_periodic(
    timeseries_t const& ts, typename timeseries_t::duration_type delay) {
  typedef typename timeseries_t::extend_by_recycling recycle;
  typedef typename timeseries_t::duration_type duration_t;

  timeseries_t a(ts.sampling_period(), ts.initial_timestamp(), ts.size());

  for (std::size_t i = 0; i != a.size(); ++i) {
    duration_t stamp = a.sampling_period_start(i) - delay;
    a[i] = ts.at(stamp, recycle());
  }
  return std::move(a);
}

/**
 * Delay a timeseries using zeroes for early values.
 */
template<typename timeseries_t>
timeseries_t delay_timeseries_zeroes(
    timeseries_t const& ts, typename timeseries_t::duration_type delay) {
  typedef typename timeseries_t::extend_by_zeroes zeroes;
  typedef typename timeseries_t::duration_type duration_t;

  timeseries_t a(ts.sampling_period(), ts.initial_timestamp(), ts.size());

  for (std::size_t i = 0; i != a.size(); ++i) {
    duration_t stamp = a.sampling_period_start(i) - delay;
    a[i] = ts.at(stamp, zeroes());
  }
  return std::move(a);
}

} // namespace testing
} // namespace jb

#endif // jb_testing_delay_timeseries_hpp
