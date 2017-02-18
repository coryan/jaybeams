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

  // ... create some source data to feed into the test ...
  std::vector<T> host(dsize);
  T gen = T(10.0);
  std::generate(host.begin(), host.end(), [&gen]() {
    gen += T(1.0);
    return gen;
  });
  // jb::testing::create_random_timeseries(generator, dsize, host);
  std::vector<T> expected = host;

  // ... upload the data to the device (asynchronously) ...
  boost::compute::vector<T> dev(dsize, context);
  // ... set the device vector to a known state for easier debugging ...
  {
    std::vector<T> tmp(dsize);
    std::fill(tmp.begin(), tmp.end(), T(2.0));
    boost::compute::copy(tmp.begin(), tmp.end(), dev.begin(), upqueue);
  }
  // ... create a buffer to receive the data on the host ...
  std::vector<T> actual(hsize);
  // ... also set it to a known state for easier debugging ...
  std::fill(actual.begin(), actual.end(), T(3.0));

  // ... copy the data from host to dev, asynchronously ...
  auto upload = boost::compute::copy_async(
      host.begin(), host.end(), dev.begin(), upqueue);
  if (not upload.valid()) {
    BOOST_CHECK_EQUAL(dsize, 0);
    return;
  }

  // ... before the upload copy ends, start the download copy ...
  auto done = jb::opencl::copy_to_host_async(
      dev.begin(), dev.end(), actual.begin(), dnqueue,
      boost::compute::wait_list(upload.get_event()));
  BOOST_REQUIRE(done.valid());
  BOOST_REQUIRE_NE(hsize, 0);

  // ... wait for the download to complete ...
  done.wait();

  // ... the upload should be done too ...
  BOOST_REQUIRE_EQUAL(
      upload.get_event().status(),
      boost::compute::event::execution_status::complete);
  BOOST_REQUIRE_EQUAL(
      done.get_event().status(),
      boost::compute::event::execution_status::complete);

  // ... make sure the received size is correct ...
  BOOST_REQUIRE_EQUAL(done.get() - actual.begin(), std::size_t(dsize));
  // ... resize the receiving buffer based on the returned iterator ...
  actual.resize(done.get() - actual.begin());

  // ... validate the contents ...
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
