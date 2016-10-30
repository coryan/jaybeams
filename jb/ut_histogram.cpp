#include <jb/histogram.hpp>

#include <boost/test/unit_test.hpp>

#include <limits>

/**
 * Helper types and objects to test the jb::histogram class.
 */
namespace {

double t_interpolate(double x_a, double x_b, double y_a, double s, double q) {
  return x_a + (q - y_a) * (x_b - x_a) / s;
}

int t_interpolate(int x_a, int x_b, double y_a, double s, double q) {
  return int(std::floor(x_a + (q - y_a) * (x_b - x_a) / s));
}

template <typename T> struct test_binning {
  typedef T sample_type;

  static sample_type const min;
  static sample_type const max;

  sample_type histogram_min() const {
    return min;
  }
  sample_type histogram_max() const {
    return max;
  }
  sample_type theoretical_min() const {
    return std::numeric_limits<int>::min();
  }
  sample_type theoretical_max() const {
    return std::numeric_limits<int>::max();
  }
  std::size_t sample2bin(sample_type x) const {
    if (x < min or x > max) {
      throw std::invalid_argument("Invalid sample value in sample2bin()");
    }
    return static_cast<std::size_t>(x - min);
  }
  sample_type bin2sample(std::size_t i) const {
    return min + i;
  }
  sample_type interpolate(sample_type x_a, sample_type x_b, double y_a,
                          double s, double q) const {
    return t_interpolate(x_a, x_b, y_a, s, q);
  }
};

template <typename T> T const test_binning<T>::min = 20;
template <typename T> T const test_binning<T>::max = 30;

typedef jb::histogram<test_binning<int>> tested_histogram;

} // anonymous namespace

/**
 * @test Verify that a simple histogram can be initialized.
 */
BOOST_AUTO_TEST_CASE(histogram_initialization) {
  tested_histogram h;
  BOOST_CHECK_EQUAL(h.nsamples(), 0);
  BOOST_CHECK_EQUAL(h.underflow_count(), 0);
  BOOST_CHECK_EQUAL(h.overflow_count(), 0);
}

/**
 * @test Verify that a simple histogram underflow operations work.
 */
BOOST_AUTO_TEST_CASE(histogram_underflow) {
  tested_histogram h;
  h.sample(10);
  BOOST_CHECK_EQUAL(h.nsamples(), 1);
  BOOST_CHECK_EQUAL(h.underflow_count(), 1);
  BOOST_CHECK_EQUAL(h.overflow_count(), 0);
  BOOST_CHECK_EQUAL(h.observed_min(), 10);
  BOOST_CHECK_EQUAL(h.observed_max(), 10);

  h.sample(5);
  h.sample(5);
  h.sample(5);
  BOOST_CHECK_EQUAL(h.nsamples(), 4);
  BOOST_CHECK_EQUAL(h.underflow_count(), 4);
  BOOST_CHECK_EQUAL(h.overflow_count(), 0);
  BOOST_CHECK_EQUAL(h.observed_min(), 5);
  BOOST_CHECK_EQUAL(h.observed_max(), 10);

  h.weighted_sample(5, 2);
  BOOST_CHECK_EQUAL(h.nsamples(), 6);
  BOOST_CHECK_EQUAL(h.underflow_count(), 6);
  BOOST_CHECK_EQUAL(h.overflow_count(), 0);
  BOOST_CHECK_EQUAL(h.observed_min(), 5);
  BOOST_CHECK_EQUAL(h.observed_max(), 10);

  h.weighted_sample(50, 0);
  BOOST_CHECK_EQUAL(h.nsamples(), 6);
  BOOST_CHECK_EQUAL(h.underflow_count(), 6);
  BOOST_CHECK_EQUAL(h.overflow_count(), 0);
  BOOST_CHECK_EQUAL(h.observed_min(), 5);
  BOOST_CHECK_EQUAL(h.observed_max(), 10);
}

/**
 * @test Verify that a simple histogram overflow operations work.
 */
BOOST_AUTO_TEST_CASE(histogram_overflow) {
  tested_histogram h;
  h.sample(40);
  BOOST_CHECK_EQUAL(h.nsamples(), 1);
  BOOST_CHECK_EQUAL(h.underflow_count(), 0);
  BOOST_CHECK_EQUAL(h.overflow_count(), 1);
  BOOST_CHECK_EQUAL(h.observed_min(), 40);
  BOOST_CHECK_EQUAL(h.observed_max(), 40);

  h.sample(45);
  h.sample(45);
  h.sample(45);
  BOOST_CHECK_EQUAL(h.nsamples(), 4);
  BOOST_CHECK_EQUAL(h.underflow_count(), 0);
  BOOST_CHECK_EQUAL(h.overflow_count(), 4);
  BOOST_CHECK_EQUAL(h.observed_min(), 40);
  BOOST_CHECK_EQUAL(h.observed_max(), 45);

  h.weighted_sample(45, 2);
  BOOST_CHECK_EQUAL(h.nsamples(), 6);
  BOOST_CHECK_EQUAL(h.underflow_count(), 0);
  BOOST_CHECK_EQUAL(h.overflow_count(), 6);
  BOOST_CHECK_EQUAL(h.observed_min(), 40);
  BOOST_CHECK_EQUAL(h.observed_max(), 45);
}

/**
 * @test Verify that the mean estimator works as expected.
 */
BOOST_AUTO_TEST_CASE(histogram_mean_simple) {
  tested_histogram h;
  BOOST_CHECK_THROW(h.estimated_mean(), std::invalid_argument);
  h.sample(25);
  h.sample(25);
  h.sample(25);
  h.sample(25);
  BOOST_CHECK_EQUAL(h.estimated_mean(), 25);
  h.sample(27);
  h.sample(27);
  h.sample(27);
  h.sample(27);
  BOOST_CHECK_EQUAL(h.estimated_mean(), 26);
}

/**
 * @test Verify that the mean estimator works as expected.
 */
BOOST_AUTO_TEST_CASE(histogram_mean_underflow) {
  tested_histogram h;
  h.sample(0);
  h.sample(0);
  h.sample(0);
  BOOST_CHECK_EQUAL(h.estimated_mean(), 10);
}

/**
 * @test Verify that the mean estimator works as expected.
 */
BOOST_AUTO_TEST_CASE(histogram_mean_overflow) {
  tested_histogram h;
  h.sample(40);
  h.sample(40);
  h.sample(40);
  BOOST_CHECK_EQUAL(h.estimated_mean(), 35);
}

/**
 * @test Verify that the mean estimator works as expected.
 */
BOOST_AUTO_TEST_CASE(histogram_mean_complex) {
  tested_histogram h;
  h.sample(0);
  h.sample(0);
  h.weighted_sample(20, 3);
  h.sample(50);
  BOOST_CHECK_EQUAL(h.estimated_mean(), 20);
}

/**
 * @test Verify that the quantile estimator works as expected.
 */
BOOST_AUTO_TEST_CASE(histogram_quantile_simple) {
  tested_histogram h;
  BOOST_CHECK_THROW(h.estimated_quantile(0.0), std::invalid_argument);

  h.sample(20);
  BOOST_CHECK_EQUAL(h.estimated_quantile(0.0), 20);
  BOOST_CHECK_EQUAL(h.estimated_quantile(0.5), 20);
  BOOST_CHECK_EQUAL(h.estimated_quantile(1.0), 21);

  BOOST_CHECK_THROW(h.estimated_quantile(-1.0), std::invalid_argument);
  BOOST_CHECK_THROW(h.estimated_quantile(2.00), std::invalid_argument);

  h.sample(21);
  h.sample(22);
  BOOST_CHECK_EQUAL(h.estimated_quantile(0.0), 20);
  BOOST_CHECK_EQUAL(h.estimated_quantile(0.5), 21);
  BOOST_CHECK_EQUAL(h.estimated_quantile(1.0), 23);

  h.sample(23);
  h.sample(24);
  h.sample(25);
  h.sample(26);
  h.sample(27);
  h.sample(28);
  BOOST_CHECK_EQUAL(h.estimated_quantile(0.00), 20);
  BOOST_CHECK_EQUAL(h.estimated_quantile(0.25), 22);
  BOOST_CHECK_EQUAL(h.estimated_quantile(0.50), 24);
  BOOST_CHECK_EQUAL(h.estimated_quantile(0.75), 26);
  BOOST_CHECK_EQUAL(h.estimated_quantile(1.00), 29);
}

/**
 * @test Verify that the quantile estimator works as expected.
 */
BOOST_AUTO_TEST_CASE(histogram_quantile_float) {
  typedef jb::histogram<test_binning<double>> double_histogram;
  double_histogram h;

  BOOST_CHECK_THROW(h.estimated_quantile(0), std::invalid_argument);

  h.sample(20);
  double eps = 100 * std::numeric_limits<double>::epsilon();
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.00), 20.00, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.25), 20.25, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.50), 20.50, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.75), 20.75, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(1.00), 21.00, eps);

  h.sample(21);
  h.sample(22);
  h.sample(23);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.00), 20.0, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.25), 21.0, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.50), 22.0, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.75), 23.0, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(1.00), 24.0, eps);

  h.sample(24);
  h.sample(25);
  h.sample(26);
  h.sample(27);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.00), 20.0, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.25), 22.0, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.50), 24.0, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.75), 26.0, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(1.00), 28.0, eps);
}

BOOST_AUTO_TEST_CASE(histogram_quantile_underflow) {
  typedef jb::histogram<test_binning<double>> double_histogram;
  double_histogram h;
  double eps = 100 * std::numeric_limits<double>::epsilon();

  h.sample(10);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.00), 10.0, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.25), 12.5, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.50), 15.0, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.75), 17.5, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(1.00), 20.0, eps);
}

BOOST_AUTO_TEST_CASE(histogram_quantile_overflow) {
  typedef jb::histogram<test_binning<double>> double_histogram;
  double_histogram h;
  double eps = 100 * std::numeric_limits<double>::epsilon();

  h.sample(40);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.00), 30.0, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.25), 32.5, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.50), 35.0, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.75), 37.5, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(1.00), 40.0, eps);
}

BOOST_AUTO_TEST_CASE(histogram_quantile_complex) {
  typedef jb::histogram<test_binning<double>> double_histogram;
  double_histogram h;
  double eps = 100 * std::numeric_limits<double>::epsilon();

  // i       : 10 20 21 22 23 24 25  26  27  28  29  30  40
  // sum(N_i):  0  1  4  4  4  4  4   9   9   9   9   9  10
  // cum_dens:  0 .1 .4 .5 .6 .7 .8 .82 .84 .86 .88  .9 1.0

  h.sample(10);
  h.sample(20);
  h.sample(20);
  h.sample(20);
  h.sample(20);
  h.sample(25);
  h.sample(25);
  h.sample(25);
  h.sample(25);
  h.sample(40);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.00), 10.0, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.05), 15.0, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.10), 20.0, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.25), 20.375, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.50), 21.0, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.90), 26.0, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.95), 35.0, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(1.00), 40.0, eps);
}
