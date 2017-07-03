#include <jb/opencl/build_simple_kernel.hpp>
#include <jb/opencl/device_selector.hpp>

#include <boost/compute/context.hpp>
#include <boost/test/unit_test.hpp>
#include <sstream>

// Define a program with two kernels, nothing fancy
char const valid_program[] = R"""(
__kernel void add_float(
    __global float *dst, __global float const *src, unsigned int const N) {
  int row = get_global_id(0);
  if (row < N) {
    dst[row] = dst[row] + src[row];
  }
}

__kernel void add_int(
    __global int *dst, __global int const *src,
    unsigned int const N) {
  int row = get_global_id(0);
  if (row < N) {
    dst[row] = dst[row] + src[row];
  }
}
)""";

// Define an invalid program
char const invalid_program[] = R"""(
__kernel void add_float(
    __global float *dst, __global float const* src, unsigned int const N) {
  int row = get_global_id(0);
  if (row < N) {
    dest[row] = dst[row] + src[row]; /* oops typo in the lhs */
  }
}
)""";

/**
 * @test Make sure jb::opencl::build_simple_kernel works as expected.
 */
BOOST_AUTO_TEST_CASE(build_simple_kernel) {
  boost::compute::device device = jb::opencl::device_selector();
  BOOST_TEST_MESSAGE("Running with device=" << device.name());
  boost::compute::context context(device);

  BOOST_CHECK_NO_THROW(jb::opencl::build_simple_kernel(
      context, device, valid_program, "add_int"));
  BOOST_CHECK_NO_THROW(jb::opencl::build_simple_kernel(
      context, device, valid_program, "add_float"));
  BOOST_CHECK_THROW(
      jb::opencl::build_simple_kernel(
          context, device, invalid_program, "add_float"),
      std::exception);

  std::istringstream is(valid_program);
  BOOST_CHECK_NO_THROW(
      jb::opencl::build_simple_kernel(context, device, is, "add_int"));
  is.str(valid_program);
  is.clear();
  BOOST_CHECK_NO_THROW(
      jb::opencl::build_simple_kernel(context, device, is, "add_float"));
  is.str(invalid_program);
  is.clear();
  BOOST_CHECK_THROW(
      jb::opencl::build_simple_kernel(context, device, is, "add_float"),
      std::exception);
}

/**
 * @test Make sure jb::opencl::build_simple_program works as expected.
 */
BOOST_AUTO_TEST_CASE(build_simple_program) {
  boost::compute::device device = jb::opencl::device_selector();
  BOOST_TEST_MESSAGE("Running with device=" << device.name());
  boost::compute::context context(device);

  boost::compute::program program;
  BOOST_CHECK_NO_THROW(
      program =
          jb::opencl::build_simple_program(context, device, valid_program));
  BOOST_CHECK_NO_THROW(boost::compute::kernel(program, "add_float"));
  BOOST_CHECK_NO_THROW(boost::compute::kernel(program, "add_int"));

  BOOST_CHECK_THROW(
      jb::opencl::build_simple_program(context, device, invalid_program),
      std::exception);

  std::istringstream is(valid_program);
  BOOST_CHECK_NO_THROW(
      program = jb::opencl::build_simple_program(context, device, is));
  BOOST_CHECK_NO_THROW(boost::compute::kernel(program, "add_float"));
  BOOST_CHECK_NO_THROW(boost::compute::kernel(program, "add_int"));

  is.str(invalid_program);
  is.clear();
  BOOST_CHECK_THROW(
      jb::opencl::build_simple_program(context, device, is), std::exception);
}
