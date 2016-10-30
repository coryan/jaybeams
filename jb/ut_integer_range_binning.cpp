#include <jb/integer_range_binning.hpp>

#include <boost/test/unit_test.hpp>

namespace {

template <typename sample_type_t> void check_constructor() {
  BOOST_CHECK_THROW(jb::integer_range_binning<sample_type_t>(10, 10),
                    std::exception);
  BOOST_CHECK_THROW(jb::integer_range_binning<sample_type_t>(20, 10),
                    std::exception);
  BOOST_CHECK_NO_THROW(jb::integer_range_binning<sample_type_t>(1, 2));
  BOOST_CHECK_NO_THROW(jb::integer_range_binning<sample_type_t>(1000, 2000));
}

template <typename sample_type_t> void check_basic() {
  jb::integer_range_binning<sample_type_t> bin(0, 1000);
  BOOST_CHECK_EQUAL(bin.histogram_min(), 0);
  BOOST_CHECK_EQUAL(bin.histogram_max(), 1000);
  BOOST_CHECK_EQUAL(bin.theoretical_min(),
                    std::numeric_limits<sample_type_t>::min());
  BOOST_CHECK_EQUAL(bin.theoretical_max(),
                    std::numeric_limits<sample_type_t>::max());
  BOOST_CHECK_EQUAL(bin.sample2bin(0), 0);
  BOOST_CHECK_EQUAL(bin.sample2bin(5), 5);
  BOOST_CHECK_EQUAL(bin.sample2bin(999), 999);
  BOOST_CHECK_EQUAL(bin.bin2sample(0), 0);
  BOOST_CHECK_EQUAL(bin.bin2sample(10), 10);
}

} // anonymous namespace

/**
 * @test Verify the constructor in jb::integer_range_binning works as expected.
 */
BOOST_AUTO_TEST_CASE(integer_range_binning_constructor_int) {
  check_constructor<int>();
}

/**
 * @test Verify that jb::integer_range_binning works as expected.
 */
BOOST_AUTO_TEST_CASE(integer_range_binning_basic_int) {
  check_basic<int>();
}

/**
 * @test Verify the constructor in jb::integer_range_binning works as expected.
 */
BOOST_AUTO_TEST_CASE(integer_range_binning_constructor_std_uint64) {
  check_constructor<std::uint64_t>();
}

/**
 * @test Verify that jb::integer_range_binning works as expected.
 */
BOOST_AUTO_TEST_CASE(integer_range_binning_basic_std_uint64) {
  check_basic<std::uint64_t>();
}
