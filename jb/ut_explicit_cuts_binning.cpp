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
