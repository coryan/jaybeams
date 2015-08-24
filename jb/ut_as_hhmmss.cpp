#include <jb/as_hhmmss.hpp>

#include <boost/test/unit_test.hpp>

namespace {

std::chrono::microseconds::rep usec = 1000000;

} // anonymous namespace

/**
 * @test Verify that hhmmssu works correctly:
 */
BOOST_AUTO_TEST_CASE(hhmmssu_ios) {
  {
    // 13:14:15.123 in microseconds
    std::chrono::microseconds t(((13 * 60 + 14) * 60 + 15) * usec + 123000);

    std::ostringstream os;
    os << jb::as_hhmmssu(t);
    BOOST_CHECK_EQUAL(os.str(), "131415.123000");
  }

  {
    // 09:05:04.000123 in microseconds
    std::chrono::microseconds t(((9 * 60 + 5) * 60 + 4) * usec + 123);

    std::ostringstream os;
    os << jb::as_hhmmssu(t);
    BOOST_CHECK_EQUAL(os.str(), "090504.000123");
  }
}

/**
 * @test Verify that hhmmss works correctly:
 */
BOOST_AUTO_TEST_CASE(hhmmss_ios) {
  {
    // 13:14:15.123 in microseconds
    std::chrono::microseconds t(((13 * 60 + 14) * 60 + 15) * usec + 123000);

    std::ostringstream os;
    os << jb::as_hhmmss(t);
    BOOST_CHECK_EQUAL(os.str(), "131415");
  }

  {
    // 09:05:02.123 in microseconds
    std::chrono::microseconds t(((9 * 60 + 5) * 60 + 2) * usec + 123000);

    std::ostringstream os;
    os << jb::as_hhmmss(t);
    BOOST_CHECK_EQUAL(os.str(), "090502");
  }
}

/**
 * @test Verify that as_hh_mm_ss_u works correctly:
 */
BOOST_AUTO_TEST_CASE(as_hh_mm_ss_u_ios) {
  {
    // 13:14:15.123 in microseconds
    std::chrono::microseconds t(((13 * 60 + 14) * 60 + 15) * usec + 123000);

    std::ostringstream os;
    os << jb::as_hh_mm_ss_u(t);
    BOOST_CHECK_EQUAL(os.str(), "13:14:15.123000");
  }

  {
    // 09:05:04.000123 in microseconds
    std::chrono::microseconds t(((9 * 60 + 5) * 60 + 4) * usec + 123);

    std::ostringstream os;
    os << jb::as_hh_mm_ss_u(t);
    BOOST_CHECK_EQUAL(os.str(), "09:05:04.000123");
  }
}
