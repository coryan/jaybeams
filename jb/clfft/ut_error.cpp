#include <jb/clfft/error.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::opencl::check_error_code works as expected.
 */
BOOST_AUTO_TEST_CASE(check_error_code) {
  BOOST_CHECK_NO_THROW(jb::clfft::check_error_code(CL_SUCCESS, "foo"));
  BOOST_CHECK_THROW(
      jb::clfft::check_error_code(CL_DEVICE_NOT_FOUND, "bar"),
      std::runtime_error);
}
