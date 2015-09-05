#include <jb/testing/delay_timeseries.hpp>
#include <jb/testing/create_triangle_timeseries.hpp>

#include <boost/test/unit_test.hpp>
#include <chrono>

namespace std {

// Horrible hack to make Boost.Test happy
std::ostream& operator<<(
    std::ostream& os, std::pair<std::ptrdiff_t,float> const& x) {
  return os << "{" << x.first << "," << x.second << "}";
}

} // namespace std

/**
 * @test Test jb::testing::delay_timeseries_periodic with default types
 *
 * Usually I do not spend time testing helper functions for tests, but
 * in this case the helper has a lot of template code and we want to
 * validate this before too long.
 */
BOOST_AUTO_TEST_CASE(delay_timeseries_periodic_default) {
  int delay = 23;
  std::vector<float> ts;
  jb::testing::create_triangle_timeseries(1024, ts);
  auto delayed = jb::testing::delay_timeseries_periodic(
      ts, std::chrono::microseconds(delay), std::chrono::microseconds(1));

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
 * @test Improve coverage for jb::testing::extrapolate_periodic.
 */
BOOST_AUTO_TEST_CASE(extrapolate_periodic) {
  jb::testing::extrapolate_periodic<float> tested;
  BOOST_CHECK_EQUAL(
      tested(0, 0), std::make_pair(std::ptrdiff_t(0), float(0)));
  BOOST_CHECK_EQUAL(
      tested(1, 100), std::make_pair(std::ptrdiff_t(1), float(0)));
  BOOST_CHECK_EQUAL(
      tested(120, 100), std::make_pair(std::ptrdiff_t(20), float(0)));
  BOOST_CHECK_EQUAL(
      tested(-20, 100), std::make_pair(std::ptrdiff_t(80), float(0)));
}

/**
 * @test Test jb::testing::delay_timeseries_zeroes with default types
 *
 * Usually I do not spend time testing helper functions for tests, but
 * in this case the helper has a lot of template code and we want to
 * validate this before too long.
 */
BOOST_AUTO_TEST_CASE(delay_timeseries_zeroes_default) {
  int delay = 23;
  std::vector<float> ts;
  jb::testing::create_triangle_timeseries(1024, ts);
  auto delayed = jb::testing::delay_timeseries_zeroes(
      ts, std::chrono::microseconds(delay), std::chrono::microseconds(1));

  for (std::size_t i = 0; i != std::size_t(delay); ++i) {
    BOOST_CHECK_SMALL(delayed.at(i), 1.0f / 1024);
  }
  for (std::size_t i = delay; i != delayed.size() - delay; ++i) {
    float expected = ts.at(i - delay);
    BOOST_CHECK_CLOSE(expected, delayed.at(i), 1.0 / 1024);
  }
}
