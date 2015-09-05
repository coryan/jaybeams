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

  timeseries_type a;
  jb::testing::create_square_timeseries(nsamples, a);
  timeseries_type b = jb::testing::delay_timeseries_periodic(
      a, std::chrono::microseconds(delay), std::chrono::microseconds(1));

  tested estimator(a, b);
  auto e = estimator.estimate_delay(a, b);
  BOOST_CHECK_EQUAL(e.first, true);
  BOOST_CHECK_CLOSE(e.second, double(delay), 0.01);
}
