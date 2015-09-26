#include <jb/opencl/device_selector.hpp>
#include <jb/opencl/copy_to_host_async.hpp>
#include <jb/testing/check_vector_close_enough.hpp>
#include <jb/testing/create_random_timeseries.hpp>

#include <boost/compute/algorithm/max_element.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/context.hpp>
#include <boost/compute/types/complex.hpp>
#include <boost/compute/lambda/placeholders.hpp>
#include <boost/test/unit_test.hpp>
#include <random>
#include <sstream>

/**
 * @test Test max_element for std::complex<float> 
 */
BOOST_AUTO_TEST_CASE(conjugate_and_multiply_float) {
  constexpr std::size_t size = 32768;
  typedef float precision_t;

  boost::compute::device device = jb::opencl::device_selector();
  boost::compute::context context(device);
  boost::compute::command_queue queue(context, device);

  unsigned int seed = std::random_device()();
  std::mt19937 gen(seed);
  std::uniform_real_distribution<precision_t> dis(-1000, 1000);
  auto generator = [&gen, &dis]() { return dis(gen); };
  BOOST_MESSAGE("SEED = " << seed);

  std::vector<std::complex<precision_t>> src;
  jb::testing::create_random_timeseries(generator, size, src);

  boost::compute::vector<std::complex<precision_t>> a(size, context);

  boost::compute::copy(
      src.begin(), src.end(), a.begin(), queue);

  typedef std::complex<precision_t> value_type;
  BOOST_COMPUTE_FUNCTION(
      bool, less_real, (value_type const& a, value_type const& b),
      { return a.x < b.x; });

  auto actual = boost::compute::max_element(
      a.begin(), a.end(), less_real, queue);

  auto pos = std::max_element(
      src.begin(), src.end(),
      [](value_type const& a, value_type const& b) {
        return real(a) < real(b);
      });

  auto expected = std::distance(src.begin(), pos);
  BOOST_CHECK_EQUAL(
      expected, std::distance(a.begin(), actual));
  BOOST_MESSAGE(
      "Maximum found at " << expected << " value = " << src[expected]);
                
}

