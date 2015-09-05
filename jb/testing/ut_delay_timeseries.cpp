#include <jb/testing/delay_timeseries.hpp>
#include <jb/testing/create_triangle_timeseries.hpp>

#include <boost/test/unit_test.hpp>
#include <complex>

/**
 * @test Test jb::testing::delay_timeseries_periodic with default types
 *
 * Usually I do not spend time testing helper functions for tests, but
 * in this case the helper has a lot of template code and we want to
 * validate this before too long.
 */
BOOST_AUTO_TEST_CASE(delay_timeseries_periodic_default) {
  int delay = 23;
  jb::timeseries<float,std::chrono::microseconds> ts(
      std::chrono::microseconds(1), std::chrono::microseconds(0));
  jb::testing::create_triangle_timeseries(1024, ts);
  auto delayed = jb::testing::delay_timeseries_periodic(
      ts, std::chrono::microseconds(delay));

  for (std::size_t i = 0; i != std::size_t(delay); ++i) {
    float expected = ts.at(ts.size() + i - delay);
    BOOST_CHECK_CLOSE(expected, delayed.at(i), 1.0 / 1024);
  }
  for (std::size_t i = delay; i != delayed.size(); ++i) {
    float expected = ts.at(i - delay);
    BOOST_CHECK_CLOSE(expected, delayed.at(i), 1.0 / 1024);
  }
}

/**
 * @test Test jb::testing::delay_timeseries_zeroes with default types
 *
 * Usually I do not spend time testing helper functions for tests, but
 * in this case the helper has a lot of template code and we want to
 * validate this before too long.
 */
BOOST_AUTO_TEST_CASE(delay_timeseries_zeroes_default) {
  int delay = 3;
  jb::timeseries<float,std::chrono::microseconds> ts(
      std::chrono::microseconds(1), std::chrono::microseconds(0));
  jb::testing::create_triangle_timeseries(1024, ts);
  auto delayed = jb::testing::delay_timeseries_zeroes(
      ts, std::chrono::microseconds(delay));

  for (std::size_t i = 0; i != std::size_t(delay); ++i) {
    BOOST_CHECK_SMALL(delayed.at(i), 1.0f / 1024);
  }
  for (std::size_t i = delay; i != delayed.size() - delay; ++i) {
    float expected = ts.at(i - delay);
    BOOST_CHECK_CLOSE(expected, delayed.at(i), 1.0 / 1024);
  }
}
