#include <jb/strtonum.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that strtonum<int> works as expected.
 */
BOOST_AUTO_TEST_CASE(strtonum_int) {
  int actual;

  BOOST_CHECK_EQUAL(false, jb::strtonum("", actual));
  BOOST_CHECK_EQUAL(true, jb::strtonum("1234", actual));
  BOOST_CHECK_EQUAL(1234, actual);
  BOOST_CHECK_EQUAL(true, jb::strtonum("-1234", actual));
  BOOST_CHECK_EQUAL(-1234, actual);
  BOOST_CHECK_EQUAL(true, jb::strtonum(" 2345", actual));
  BOOST_CHECK_EQUAL(2345, actual);

  BOOST_CHECK_EQUAL(false, jb::strtonum("3456 ", actual));
  BOOST_CHECK_EQUAL(2345, actual);
  BOOST_CHECK_EQUAL(false, jb::strtonum(" 3456 ", actual));
  BOOST_CHECK_EQUAL(2345, actual);
  BOOST_CHECK_EQUAL(false, jb::strtonum("xyxxz", actual));
  BOOST_CHECK_EQUAL(2345, actual);
  BOOST_CHECK_EQUAL(false, jb::strtonum("123.45", actual));
  BOOST_CHECK_EQUAL(2345, actual);
  BOOST_CHECK_EQUAL(false, jb::strtonum("123-7", actual));
  BOOST_CHECK_EQUAL(2345, actual);
  BOOST_CHECK_EQUAL(false, jb::strtonum("123adbcd", actual));
  BOOST_CHECK_EQUAL(2345, actual);
  BOOST_CHECK_EQUAL(false, jb::strtonum("20000000000000000000", actual));
  BOOST_CHECK_EQUAL(2345, actual);
}

/**
 * @test Verify that strtonum<double> works as expected.
 */
BOOST_AUTO_TEST_CASE(strtonum_double) {
  double actual;

  BOOST_CHECK_EQUAL(false, jb::strtonum("", actual));
  BOOST_CHECK_EQUAL(true, jb::strtonum("1234", actual));
  BOOST_CHECK_EQUAL(1234, actual);
  BOOST_CHECK_EQUAL(true, jb::strtonum("-1234", actual));
  BOOST_CHECK_EQUAL(-1234, actual);
  BOOST_CHECK_EQUAL(true, jb::strtonum("-1234.56", actual));
  BOOST_CHECK_EQUAL(-1234.56, actual);
  BOOST_CHECK_EQUAL(true, jb::strtonum("0.125", actual));
  BOOST_CHECK_EQUAL(0.125, actual);
  BOOST_CHECK_EQUAL(true, jb::strtonum(".5", actual));
  BOOST_CHECK_EQUAL(0.5, actual);
  BOOST_CHECK_EQUAL(true, jb::strtonum(" 2345", actual));
  BOOST_CHECK_EQUAL(2345, actual);

  BOOST_CHECK_EQUAL(false, jb::strtonum("3456 ", actual));
  BOOST_CHECK_EQUAL(2345, actual);
  BOOST_CHECK_EQUAL(false, jb::strtonum(" 3456 ", actual));
  BOOST_CHECK_EQUAL(2345, actual);
  BOOST_CHECK_EQUAL(false, jb::strtonum("xyxxz", actual));
  BOOST_CHECK_EQUAL(2345, actual);
  BOOST_CHECK_EQUAL(false, jb::strtonum("123-7", actual));
  BOOST_CHECK_EQUAL(2345, actual);
  BOOST_CHECK_EQUAL(false, jb::strtonum("123adbcd", actual));
  BOOST_CHECK_EQUAL(2345, actual);
}
