#include <jb/opencl/copy_to_host_async.hpp>
#include <jb/opencl/device_selector.hpp>
#include <jb/tde/conjugate_and_multiply.hpp>
#include <jb/testing/check_vector_close_enough.hpp>
#include <jb/testing/create_random_timeseries.hpp>

#include <boost/compute/context.hpp>
#include <boost/test/unit_test.hpp>
#include <random>
#include <sstream>

namespace {

template <typename precision_t>
void check_conjugate_and_multiply() {
  boost::compute::device device = jb::opencl::device_selector();
  boost::compute::context context(device);
  boost::compute::command_queue queue(context, device);

  unsigned int seed = std::random_device()();
  std::mt19937 gen(seed);
  std::uniform_real_distribution<precision_t> dis(-1000, 1000);
  auto generator = [&gen, &dis]() { return dis(gen); };
  BOOST_TEST_MESSAGE("SEED = " << seed);

  constexpr std::size_t size = 32768;
  std::vector<std::complex<precision_t>> asrc;
  jb::testing::create_random_timeseries(generator, size, asrc);
  std::vector<std::complex<precision_t>> bsrc;
  jb::testing::create_random_timeseries(generator, size, bsrc);

  boost::compute::vector<std::complex<precision_t>> a(size, context);
  boost::compute::vector<std::complex<precision_t>> b(size, context);
  boost::compute::vector<std::complex<precision_t>> out(size, context);
  std::vector<std::complex<precision_t>> actual(size);

  boost::compute::copy(asrc.begin(), asrc.end(), a.begin(), queue);
  boost::compute::copy(bsrc.begin(), bsrc.end(), b.begin(), queue);

  auto future = jb::tde::conjugate_and_multiply(
      a.begin(), a.end(), b.begin(), b.end(), out.begin(), queue);
  auto done = jb::opencl::copy_to_host_async(
      out.begin(), out.end(), actual.begin(), queue,
      boost::compute::wait_list(future.get_event()));

  std::vector<std::complex<precision_t>> expected(size);
  for (std::size_t i = 0; i != size; ++i) {
    expected[i] = std::conj(asrc[i]) * bsrc[i];
  }
  done.wait();

  jb::testing::check_vector_close_enough(actual, expected);
}

} // anonymous namespace

/**
 * @test Make sure the jb::tde::conjugate_and_multiply() works as
 * expected.
 */
BOOST_AUTO_TEST_CASE(conjugate_and_multiply_float) {
  check_conjugate_and_multiply<float>();
}

/**
 * @test Make sure the jb::tde::conjugate_and_multiply() works as
 * expected.
 */
BOOST_AUTO_TEST_CASE(conjugate_and_multiply_double) {
  check_conjugate_and_multiply<double>();
}
