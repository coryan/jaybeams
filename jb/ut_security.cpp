#include <jb/security.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that we can create a security directory and use it.
 */
BOOST_AUTO_TEST_CASE(security_basic) {
  jb::security security;

  BOOST_CHECK_THROW(security.str(), std::runtime_error);
}
