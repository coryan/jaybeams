#include <jb/integer_range_binning.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify the constructor in jb::integer_range_binning works as expected.
 */
BOOST_AUTO_TEST_CASE(integer_range_binning_constructor) {
  BOOST_CHECK_THROW(
      jb::integer_range_binning<int>(10, 10), std::exception);
  BOOST_CHECK_THROW(
      jb::integer_range_binning<int>(20, 10), std::exception);
  BOOST_CHECK_NO_THROW(
      jb::integer_range_binning<int>(1, 2));
  BOOST_CHECK_NO_THROW(
      jb::integer_range_binning<int>(1000, 2000));
}

/**
 * @test Verify that jb::integer_range_binning works as expected.
 */
BOOST_AUTO_TEST_CASE(integer_range_binning_basic) {
  jb::integer_range_binning<int> bin(0, 1000);
  BOOST_CHECK_EQUAL(bin.histogram_min(), 0);
  BOOST_CHECK_EQUAL(bin.histogram_max(), 1000);
  BOOST_CHECK_EQUAL(bin.theoretical_min(), std::numeric_limits<int>::min());
  BOOST_CHECK_EQUAL(bin.theoretical_max(), std::numeric_limits<int>::max());
  BOOST_CHECK_EQUAL(bin.sample2bin(0), 0);
  BOOST_CHECK_EQUAL(bin.sample2bin(5), 5);
  BOOST_CHECK_EQUAL(bin.sample2bin(999), 999);
  BOOST_CHECK_EQUAL(bin.bin2sample(0), 0);
  BOOST_CHECK_EQUAL(bin.bin2sample(10), 10);
}
