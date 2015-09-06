#include <jb/fftw/time_delay_estimator.hpp>
#include <jb/testing/create_square_timeseries.hpp>
#include <jb/testing/delay_timeseries.hpp>

#include <boost/test/unit_test.hpp>
#include <chrono>

/**
 * @test Verify that we can create and use a simple time delay estimator.
 */
BOOST_AUTO_TEST_CASE(fftw_time_delay_estimator_simple) {
  int const nsamples = 1<<15;
  int const delay = 1250;
  typedef jb::fftw::aligned_vector<float> timeseries_type;
  typedef jb::fftw::time_delay_estimator<timeseries_type> tested;

  timeseries_type a(nsamples);
  timeseries_type b(nsamples);
  tested estimator(a, b);

  jb::testing::create_square_timeseries(nsamples, a);
  b = jb::testing::delay_timeseries_periodic(
      a, std::chrono::microseconds(delay), std::chrono::microseconds(1));

  auto e = estimator.estimate_delay(a, b);
  BOOST_CHECK_EQUAL(e.first, true);
  BOOST_CHECK_CLOSE(e.second, double(delay), 0.01);
}

/**
 * @test Verify that jb::fftw::time_delay_estimator can handle edge conditions.
 */
BOOST_AUTO_TEST_CASE(fftw_time_delay_estimator_error) {
  int const nsamples = 1<<15;
  typedef jb::fftw::aligned_vector<float> timeseries_type;
  typedef jb::fftw::time_delay_estimator<timeseries_type> tested;

  timeseries_type a = {0};
  timeseries_type b = a;

  tested estimator(a, b);
  auto e = estimator.estimate_delay(a, b);
  BOOST_CHECK_EQUAL(e.first, false);

  b.resize(nsamples / 2);
  BOOST_CHECK_THROW(estimator.estimate_delay(a, b), std::exception);

  BOOST_CHECK_THROW(tested tmp(a, b), std::exception);
}
