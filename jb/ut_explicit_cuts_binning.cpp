#include <jb/explicit_cuts_binning.hpp>
#include <jb/histogram.hpp>

#include <boost/test/unit_test.hpp>

namespace {

typedef jb::histogram<jb::explicit_cuts_binning<int>> tested_histogram;

} // anonymous namespace

/**
 * @test Verify the constructor in jb::explicit_cuts_binning works as expected.
 */
BOOST_AUTO_TEST_CASE(explicit_cuts_binning_constructor) {
  std::vector<int> cuts;
  BOOST_CHECK_THROW(
      jb::explicit_cuts_binning<int>(cuts.begin(), cuts.end()), std::exception);
  BOOST_CHECK_THROW(
      jb::explicit_cuts_binning<int>({10}), std::exception);
  BOOST_CHECK_NO_THROW(
      jb::explicit_cuts_binning<int>({1, 2}));
  BOOST_CHECK_NO_THROW(
      jb::explicit_cuts_binning<int>({1, 2, 3, 4}));
  BOOST_CHECK_THROW(
      jb::explicit_cuts_binning<int>({1, 2, 5, 4}), std::exception);
  BOOST_CHECK_THROW(
      jb::explicit_cuts_binning<int>({1, 2, 2, 4}), std::exception);
}

/**
 * @test Verify the constructor in jb::explicit_cuts_binning works as expected.
 */
BOOST_AUTO_TEST_CASE(explicit_cuts_binning_basic) {
  std::vector<int> cuts{
    0, 10, 20, 30, 40, 50, 60, 70, 80, 90,
        100, 200, 300, 400, 500, 600, 700, 800, 900, 1000};

  jb::explicit_cuts_binning<int> bin(cuts.begin(), cuts.end());
  BOOST_CHECK_EQUAL(bin.histogram_min(), 0);
  BOOST_CHECK_EQUAL(bin.histogram_max(), 1000);
  BOOST_CHECK_EQUAL(bin.theoretical_min(), std::numeric_limits<int>::min());
  BOOST_CHECK_EQUAL(bin.theoretical_max(), std::numeric_limits<int>::max());
  BOOST_CHECK_EQUAL(bin.sample2bin(0), 0);
  BOOST_CHECK_EQUAL(bin.sample2bin(5), 0);
  BOOST_CHECK_EQUAL(bin.sample2bin(25), 2);
  BOOST_CHECK_EQUAL(bin.sample2bin(90), 9);
  BOOST_CHECK_EQUAL(bin.sample2bin(99), 9);
  BOOST_CHECK_EQUAL(bin.sample2bin(100), 10);
  BOOST_CHECK_EQUAL(bin.sample2bin(120), 10);
  BOOST_CHECK_EQUAL(bin.sample2bin(193), 10);
  BOOST_CHECK_EQUAL(bin.sample2bin(400), 13);
  BOOST_CHECK_EQUAL(bin.sample2bin(999), 18);
  BOOST_CHECK_EQUAL(bin.bin2sample(0), 0);
  BOOST_CHECK_EQUAL(bin.bin2sample(1), 10);
  BOOST_CHECK_EQUAL(bin.bin2sample(10), 100);
  BOOST_CHECK_EQUAL(bin.bin2sample(11), 200);
  BOOST_CHECK_EQUAL(bin.bin2sample(14), 500);
}

/**
 * @test Verify that jb::explicit_cuts_binning works with jb::histogram.
 */
BOOST_AUTO_TEST_CASE(explicit_cuts_binning_histogram) {
  tested_histogram h({10, 20, 30, 40, 50, 100, 150, 200});
  BOOST_CHECK_THROW(h.estimated_mean(), std::invalid_argument);

  h.sample(0);
  h.sample(10);
  h.sample(20);
  h.sample(30);
  h.sample(40);
  // each bucket is estimated at the central point
  BOOST_CHECK_EQUAL(h.estimated_mean(), 25);

  h.sample(40);
  h.sample(40);
  h.sample(100);
  h.sample(200);
  h.sample(1000);

  double eps = (1<<8) * std::numeric_limits<double>::epsilon();
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.00), 0.00, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.10), 10.00, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.20), 20.00, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.30), 29.00, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.40), 40.00, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.50), 43.00, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.60), 46.00, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.70), 50.00, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(0.80), 150.00, eps);
  BOOST_CHECK_CLOSE(h.estimated_quantile(1.00), 1000.00, eps);

  
}
