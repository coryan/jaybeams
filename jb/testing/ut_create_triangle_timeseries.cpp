#include <jb/testing/check_close_enough.hpp>
#include <jb/testing/create_triangle_timeseries.hpp>

#include <boost/test/unit_test.hpp>
#include <complex>

namespace {

template <typename value_type>
void check_create_triangle() {
  std::vector<value_type> ts;
  jb::testing::create_triangle_timeseries(1024, ts);
  value_type sum = 0;
  for (auto const& i : ts) {
    sum += i;
  }
  jb::testing::check_small(sum, 1.0 / ts.size());
}

} // anonymous namespace

/**
 * @test Test jb::testing::create_triangle_timeseries with float
 *
 * Usually I do not spend time testing helper functions for tests, but
 * in this case the helper has a lot of template code and we want to
 * validate this before too long.
 */
BOOST_AUTO_TEST_CASE(create_triangle_timeseries_float) {
  check_create_triangle<float>();
}

/**
 * @test Test jb::testing::create_triangle_timeseries with double
 */
BOOST_AUTO_TEST_CASE(create_triangle_timeseries_double) {
  check_create_triangle<double>();
}

/**
 * @test Test jb::testing::create_triangle_timeseries with std::complex<float>
 */
BOOST_AUTO_TEST_CASE(create_triangle_timeseries_cfloat) {
  check_create_triangle<std::complex<float>>();
}

/**
 * @test Test jb::testing::create_triangle_timeseries with std::complex<double>
 */
BOOST_AUTO_TEST_CASE(create_triangle_timeseries_cdouble) {
  check_create_triangle<std::complex<double>>();
}
