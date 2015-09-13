#include <jb/assert_throw.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::assert_throw works as expected.
 */
BOOST_AUTO_TEST_CASE(assert_throw) {
  BOOST_CHECK_THROW(JB_ASSERT_THROW(1 == 0), std::exception);
  BOOST_CHECK_NO_THROW(JB_ASSERT_THROW(0 == 0));
}
