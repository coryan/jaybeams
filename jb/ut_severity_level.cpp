#include <jb/severity_level.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that the severity_level functions work as expected.
 */
BOOST_AUTO_TEST_CASE(severity_level_simple) {
  jb::severity_level actual = jb::severity_level::error;
  std::ostringstream os;
  os << actual;
  BOOST_CHECK_EQUAL(os.str(), std::string("ERROR"));

  std::istringstream is("NOTICE");
  is >> actual;
  BOOST_CHECK_EQUAL(actual, jb::severity_level::notice);
}

/**
 * @test Verify that the severity_level functions detect errors.
 */
BOOST_AUTO_TEST_CASE(severity_level_errors) {
  jb::severity_level in_error = static_cast<jb::severity_level>(2000);
  std::ostringstream os;
  os << in_error;
  BOOST_CHECK_EQUAL(os.str(), "[invalid severity (2000)]");

  std::istringstream is("NOT-A-SEVERITY");
  BOOST_CHECK_THROW(is >> in_error, std::exception);
}
