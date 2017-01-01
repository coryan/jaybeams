#include <jb/clfft/error.hpp>

#include <boost/test/unit_test.hpp>
#include <clFFT.h>

/**
 * @test Verify that jb::opencl::check_error_code works as expected.
 */
BOOST_AUTO_TEST_CASE(check_error_code) {
  BOOST_CHECK_NO_THROW(jb::clfft::check_error_code(CL_SUCCESS, "foo"));
  BOOST_CHECK_THROW(
      jb::clfft::check_error_code(CL_DEVICE_NOT_FOUND, "bar"),
      jb::clfft::clfft_error);
}

/**
 * @test Ensure all error codes are handled...
 */
BOOST_AUTO_TEST_CASE(clfft_error_to_string) {
  for (int error = CLFFT_BUGCHECK; error <= CLFFT_ENDSTATUS; ++error) {
    BOOST_CHECK_NE(
        jb::clfft::clfft_error::to_string(error), "unknown error code");
    BOOST_CHECK_MESSAGE(
        jb::clfft::clfft_error::to_string(error) != "unknown error code",
        "error code=" << error);
  }
  BOOST_CHECK_EQUAL(
      jb::clfft::clfft_error::to_string(CLFFT_ENDSTATUS + 1),
      "unknown error code");
  BOOST_CHECK_NE(
      jb::clfft::clfft_error::to_string(CL_SUCCESS), "unknown error code");
  BOOST_CHECK_NE(
      jb::clfft::clfft_error::to_string(CL_DEVICE_NOT_FOUND),
      "unknown error code");
}
