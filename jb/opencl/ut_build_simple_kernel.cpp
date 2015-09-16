#include <jb/opencl/build_simple_kernel.hpp>
#include <jb/opencl/device_selector.hpp>

#include <boost/test/unit_test.hpp>

// Define a program with two kernels, nothing fancy
char const valid_program[] = R"""(
__kernel void add_float(
    __global float *dst, __global float const *src, unsigned int const N) {
  int row = get_global_id(0);
  if (row < N) {
    dst[row] = dst[row] + src[row];
  }
}

__kernel void add_double(
    __global double *dst, __global double const *src,
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
  cl::Device device = jb::opencl::device_selector();
  cl::Context context = jb::opencl::device2context(device);

  BOOST_CHECK_NO_THROW(jb::opencl::build_simple_kernel(
      context, device, valid_program, "add_double"));
  BOOST_CHECK_NO_THROW(jb::opencl::build_simple_kernel(
      context, device, valid_program, "add_float"));
  BOOST_CHECK_THROW(jb::opencl::build_simple_kernel(
      context, device, invalid_program, "add_float"), std::exception);
}

/**
 * @test Make sure jb::opencl::build_simple_program works as expected.
 */
BOOST_AUTO_TEST_CASE(build_simple_program) {
  cl::Device device = jb::opencl::device_selector();
  cl::Context context = jb::opencl::device2context(device);

  cl::Program program;
  BOOST_CHECK_NO_THROW(program = jb::opencl::build_simple_program(
      context, device, valid_program));
  BOOST_CHECK_NO_THROW(cl::Kernel(program, "add_float"));
  BOOST_CHECK_NO_THROW(cl::Kernel(program, "add_double"));

  BOOST_CHECK_THROW(jb::opencl::build_simple_program(
      context, device, invalid_program), std::exception);
}
