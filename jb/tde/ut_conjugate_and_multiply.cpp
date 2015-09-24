#include <jb/tde/conjugate_and_multiply.hpp>
#include <jb/opencl/device_selector.hpp>

#include <boost/compute/context.hpp>
#include <boost/test/unit_test.hpp>
#include <sstream>

/**
 * @test Make sure we can call the operation.
 */
BOOST_AUTO_TEST_CASE(conjugate_and_multiply_operation_float) {
  boost::compute::device device = jb::opencl::device_selector();
  boost::compute::context context(device);
  boost::compute::command_queue queue(context, device);

  boost::compute::vector<std::complex<float>> a(1024);
  boost::compute::vector<std::complex<float>> b(1024);
  boost::compute::vector<std::complex<float>> dst(1024);

  jb::tde::conjugate_and_multiply(
      a.begin(), a.end(), b.begin(), b.end(), dst.begin(),
      queue);
}
