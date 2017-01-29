#include <jb/opencl/copy_to_host_async.hpp>
#include <jb/opencl/device_selector.hpp>

#include <jb/testing/check_close_enough.hpp>
#include <jb/testing/create_random_timeseries.hpp>
#include <jb/complex_traits.hpp>

#include <boost/compute/container/vector.hpp>
#include <boost/compute/context.hpp>
#include <boost/test/unit_test.hpp>
#include <random>
#include <sstream>

namespace {
/**
 * Implement a generic (on the type) test for jb::opencl::copy_to_host_async()
 *
 * @tparam T the value type stored by the buffers
 *
 * @param dsize the size of the vector on the device
 * @param hsize the size of the vector on the host that receives the data.
 */
template <typename T>
void check_copy_to_host_async_sized(int dsize, int hsize) {
  // boilerplate to connect to the OpenCL device ...
  boost::compute::device device = jb::opencl::device_selector();
  boost::compute::context context(device);
  boost::compute::command_queue upqueue(context, device);
  boost::compute::command_queue dnqueue(context, device);

  // ... more boiler place to generate more or less random numbers in
  // the test ...
  unsigned int seed = std::random_device()();
  std::mt19937 gen(seed);
  using precision_t = typename jb::extract_value_type<T>::precision;
  std::uniform_real_distribution<precision_t> dis(-1000, 1000);
  auto generator = [&gen, &dis]() { return dis(gen); };
  BOOST_TEST_MESSAGE("SEED = " << seed);

  // ... create some source data to feed into the test ...
  std::vector<T> host;
  jb::testing::create_random_timeseries(generator, dsize, host);
  std::vector<T> expected = host;

  // ... upload the data to the device (asynchronously) ...
  boost::compute::vector<T> dev(dsize, context);
  auto upload = boost::compute::copy_async(
      host.begin(), host.end(), dev.begin(), upqueue);
  if (not upload.valid()) {
    BOOST_CHECK_EQUAL(dsize, 0);
    return;
  }
  // ... create a buffer to receive the data on the host ...
  std::vector<T> actual(hsize);

  // ... copy the buffer from the device to the host, asynchronously, ...
  auto done = jb::opencl::copy_to_host_async(
      dev.begin(), dev.end(), actual.begin(), dnqueue,
      boost::compute::wait_list(upload.get_event()));
  done.wait();
  BOOST_REQUIRE_EQUAL(done.get() - actual.begin(), std::size_t(dsize));
  // ... resize the receiving buffer based on the returned iterator ...
  actual.resize(done.get() - actual.begin());

  bool res = jb::testing::check_collection_close_enough(actual, expected);
  BOOST_CHECK_MESSAGE(res, "collections are not within default tolerance");
}
} // anonymous namespace

/**
 * @test Make sure the jb::tde::copy_to_host_async() works as
 * expected.
 */
BOOST_AUTO_TEST_CASE(copy_to_host_async_float) {
  int const size = 32768;
  check_copy_to_host_async_sized<float>(size, size);
}

/**
 * @test Make sure the jb::tde::copy_to_host_async() works as
 * expected.
 */
BOOST_AUTO_TEST_CASE(copy_to_host_async_double) {
  int const size = 32768;
  check_copy_to_host_async_sized<double>(size, size);
}

/**
 * @test Make sure the jb::tde::copy_to_host_async() works as
 * expected.
 */
BOOST_AUTO_TEST_CASE(copy_to_host_async_complex_float) {
  int const size = 32768;
  check_copy_to_host_async_sized<std::complex<float>>(size, size);
}

/**
 * @test Make sure the jb::tde::copy_to_host_async() works as
 * expected.
 */
BOOST_AUTO_TEST_CASE(copy_to_host_async_complex_double) {
  int const size = 32768;
  check_copy_to_host_async_sized<std::complex<double>>(size, size);
}

/**
 * @test Make sure the jb::tde::copy_to_host_async() works for
 * empty buffers.
 */
BOOST_AUTO_TEST_CASE(copy_to_host_async_empty) {
  int const size = 10000;
  check_copy_to_host_async_sized<std::complex<float>>(0, size);
}

/**
 * @test Make sure the jb::tde::copy_to_host_async() resizes the output.
 */
BOOST_AUTO_TEST_CASE(copy_to_host_async_big_destination) {
  int const size = 8192;
  check_copy_to_host_async_sized<double>(size, 2 * size);
}

/**
 * @test Make sure the jb::tde::copy_to_host_async() works for large buffers.
 */
BOOST_AUTO_TEST_CASE(copy_to_host_async_1_24) {
  int const size = 1 << 24;
  check_copy_to_host_async_sized<double>(size, size);
}
