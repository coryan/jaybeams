#include <jb/fixed_string.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::mktdata::fixed_string works as expected.
 */
BOOST_AUTO_TEST_CASE(fixed_string_basic) {
  int constexpr size = 8;
  typedef jb::fixed_string<size> tested_type;

  tested_type t;
  t = std::string("ABC");
  BOOST_CHECK_EQUAL(t, std::string("ABC     "));
  std::ostringstream os;
  os << t;
  BOOST_CHECK_EQUAL(os.str(), "ABC     ");

  tested_type a("ABC");
  tested_type b("BCD");
  BOOST_CHECK_LT(a, b);
  BOOST_CHECK_LE(a, b);
  BOOST_CHECK_LE(a, a);
  BOOST_CHECK_GT(b, a);
  BOOST_CHECK_GE(b, a);
  BOOST_CHECK_GE(b, b);
  BOOST_CHECK_NE(a, b);
}
