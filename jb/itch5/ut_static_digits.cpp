#include <jb/itch5/static_digits.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::itch5::static_digits works as expected.
 */
BOOST_AUTO_TEST_CASE(static_digits) {
  BOOST_CHECK_EQUAL(jb::itch5::static_digits(0), 1);
  BOOST_CHECK_EQUAL(jb::itch5::static_digits(5), 1);
  BOOST_CHECK_EQUAL(jb::itch5::static_digits(9), 1);
  BOOST_CHECK_EQUAL(jb::itch5::static_digits(10), 2);
  BOOST_CHECK_EQUAL(jb::itch5::static_digits(99), 2);
  BOOST_CHECK_EQUAL(jb::itch5::static_digits(1000), 4);
  BOOST_CHECK_EQUAL(jb::itch5::static_digits(5000000), 7);
}
