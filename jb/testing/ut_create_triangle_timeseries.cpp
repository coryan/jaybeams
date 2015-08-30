#include <jb/testing/create_triangle_timeseries.hpp>

#include <boost/test/unit_test.hpp>
#include <complex>

namespace {

template<typename floating>
void check_small(floating t, double small) {
  BOOST_CHECK_SMALL(t, floating(small));
}

template<typename real>
void check_small(std::complex<real> t, double small) {
  BOOST_CHECK_SMALL(t.real(), real(small));
  BOOST_CHECK_SMALL(t.imag(), real(small));
}

template<typename expected_value_type, typename timeseries_t>
void check_create_triangle_generic(timeseries_t const& ts) {
  typedef typename timeseries_t::value_type actual_value_type;
  auto same_value_type = std::is_same<
    expected_value_type, actual_value_type>::value;
  BOOST_REQUIRE(same_value_type);
  typedef typename timeseries_t::duration_type actual_duration_type;
  auto same_duration_type =
      std::is_same<std::chrono::microseconds, actual_duration_type>::value;
  BOOST_REQUIRE(same_duration_type);
  actual_value_type sum = 0;
  for (auto const& i : ts) {
    sum += i;
  }
  check_small(sum, 1.0 / ts.size());
}

template<typename value_type>
void check_create_triangle() {
  auto ts = jb::testing::create_triangle_timeseries<value_type>(1024);

  check_create_triangle_generic<value_type>(ts);
}

} // anonymous namespace

/**
 * @test Test jb::testing::create_triangle_timeseries with default type
 *
 * Usually I do not spend time testing helper functions for tests, but
 * in this case the helper has a lot of template code and we want to
 * validate this before too long.
 */
BOOST_AUTO_TEST_CASE(create_triangle_timeseries_default) {
  auto ts = jb::testing::create_triangle_timeseries(1024);
  check_create_triangle_generic<float>(ts);
}

/**
 * @test Test jb::testing::create_triangle_timeseries with float
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
