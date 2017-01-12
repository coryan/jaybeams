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
void check_conjugate_and_multiply_sized(int asize, int bsize) {
  boost::compute::device device = jb::opencl::device_selector();
  boost::compute::context context(device);
  boost::compute::command_queue queue(context, device);

  unsigned int seed = std::random_device()();
  std::mt19937 gen(seed);
  std::uniform_real_distribution<precision_t> dis(-1000, 1000);
  auto generator = [&gen, &dis]() { return dis(gen); };
  BOOST_TEST_MESSAGE("SEED = " << seed);

  std::vector<std::complex<precision_t>> asrc;
  jb::testing::create_random_timeseries(generator, asize, asrc);
  std::vector<std::complex<precision_t>> bsrc;
  jb::testing::create_random_timeseries(generator, bsize, bsrc);

  boost::compute::vector<std::complex<precision_t>> a(asize, context);
  boost::compute::vector<std::complex<precision_t>> b(bsize, context);
  boost::compute::vector<std::complex<precision_t>> out(asize, context);
  std::vector<std::complex<precision_t>> actual(asize);

  boost::compute::copy(asrc.begin(), asrc.end(), a.begin(), queue);
  boost::compute::copy(bsrc.begin(), bsrc.end(), b.begin(), queue);

  auto future = jb::tde::conjugate_and_multiply(
      a.begin(), a.end(), b.begin(), b.end(), out.begin(), queue);
  if (not future.valid()) {
    BOOST_CHECK_EQUAL(asize, 0);
    return;
  }
  auto done = jb::opencl::copy_to_host_async(
      out.begin(), out.end(), actual.begin(), queue,
      boost::compute::wait_list(future.get_event()));

  std::vector<std::complex<precision_t>> expected(asize);
  for (int i = 0; i != asize; ++i) {
    expected[i] = std::conj(asrc[i]) * bsrc[i];
  }
  done.wait();

  int max_differences = asize / 1000;
  int result = jb::testing::check_vector_close_enough(
      actual, expected, 1, max_differences);
  BOOST_CHECK_MESSAGE(
      result >= max_differences,
      "too many differences higher than tolerance=1, result = "
          << result << ", expected less than " << max_differences);
}

template <typename precision_t>
void check_conjugate_and_multiply() {
  constexpr std::size_t size = 32768;
  check_conjugate_and_multiply_sized<precision_t>(size, size);
}

template <typename precision_t>
void check_conjugate_and_multiply_empty() {
  check_conjugate_and_multiply_sized<precision_t>(0, 0);
}

template <typename precision_t>
void check_conjugate_and_multiply_mismatched() {
  constexpr std::size_t size = 32768;
  check_conjugate_and_multiply_sized<precision_t>(size, size / 2);
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

/**
 * @test Verify that jb::tde::conjugate_and_multiply() works for empty
 * arrays.
 */
BOOST_AUTO_TEST_CASE(conjugate_and_multiply_float_empty) {
  BOOST_CHECK_NO_THROW(check_conjugate_and_multiply_empty<float>());
}

/**
 * @test Verify that jb::tde::conjugate_and_multiply() works for empty
 * arrays.
 */
BOOST_AUTO_TEST_CASE(conjugate_and_multiply_double_empty) {
  BOOST_CHECK_NO_THROW(check_conjugate_and_multiply_empty<double>());
}

/**
 * @test Verify that jb::tde::conjugate_and_multiply() detects
 * mismatched arrays.
 */
BOOST_AUTO_TEST_CASE(conjugate_and_multiply_float_error) {
  BOOST_CHECK_THROW(
      check_conjugate_and_multiply_mismatched<float>(), std::exception);
}

/**
 * @test Verify that jb::tde::conjugate_and_multiply() detects
 * mismatched arrays.
 */
BOOST_AUTO_TEST_CASE(conjugate_and_multiply_double_error) {
  BOOST_CHECK_THROW(
      check_conjugate_and_multiply_mismatched<double>(), std::exception);
}
