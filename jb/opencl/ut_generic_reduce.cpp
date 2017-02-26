#include <jb/opencl/config.hpp>
#include <jb/opencl/copy_to_host_async.hpp>
#include <jb/opencl/device_selector.hpp>
#include <jb/opencl/generic_reduce.hpp>
#include <jb/testing/check_close_enough.hpp>
#include <jb/testing/create_random_timeseries.hpp>
#include <jb/complex_traits.hpp>

#include <boost/compute/command_queue.hpp>
#include <boost/compute/container.hpp>
#include <boost/compute/context.hpp>
#include <boost/compute/type_traits.hpp>
#include <boost/compute/types/complex.hpp>
#include <boost/test/unit_test.hpp>
#include <memory>
#include <random>
#include <sstream>

namespace {

/**
 * A reducer to test.
 */
template <typename T>
class reduce_sum : public jb::opencl::generic_reduce<reduce_sum<T>, T, T> {
public:
  reduce_sum(std::size_t size, boost::compute::command_queue const& queue)
      : jb::opencl::generic_reduce<reduce_sum<T>, T, T>(size, queue) {
  }

  /// @returns the body of the initialization function
  static std::string initialize_body(char const* lhs) {
    return std::string("*") + lhs + " = 0;";
  }

  /// @returns the body of the transform function
  static std::string
  transform_body(char const* lhs, char const* value, char const*) {
    return std::string("*") + lhs + " = *" + value + ";";
  }

  /// @returns the body of the combine function
  static std::string combine_body(char const* accumulated, char const* value) {
    return std::string("*") + accumulated + " = *" + accumulated + " + *" +
           value + ";";
  }
};

} // anonymous namespace

namespace {

template <typename T>
std::function<T()> create_random_generator(unsigned int seed) {
  std::mt19937 gen(seed);
  std::uniform_int_distribution<T> dis(-1000, 1000);
  using state_type = std::pair<std::mt19937, std::uniform_int_distribution<T>>;
  std::shared_ptr<state_type> state(new state_type(gen, dis));
  return [state]() { return state->second(state->first); };
}

template <>
std::function<float()> create_random_generator<float>(unsigned int seed) {
  std::mt19937 gen(seed);
  std::uniform_real_distribution<float> dis(1, 2);
  using state_type =
      std::pair<std::mt19937, std::uniform_real_distribution<float>>;
  std::shared_ptr<state_type> state(new state_type(gen, dis));
  return [state]() { return state->second(state->first); };
}

template <>
std::function<double()> create_random_generator<double>(unsigned int seed) {
  std::mt19937 gen(seed);
  std::uniform_real_distribution<double> dis(1, 2);
  using state_type =
      std::pair<std::mt19937, std::uniform_real_distribution<double>>;
  std::shared_ptr<state_type> state(new state_type(gen, dis));
  return [state]() { return state->second(state->first); };
}

template <typename value_type>
void check_generic_reduce_sized(std::size_t size, std::size_t subset_size) {
  BOOST_TEST_MESSAGE("Testing with size = " << size);
  boost::compute::device device = jb::opencl::device_selector();
  BOOST_TEST_MESSAGE("Running on device = " << device.name());
  using scalar_type = typename jb::extract_value_type<value_type>::precision;
  if (std::is_same<double, scalar_type>::value) {
    if (not device.supports_extension("cl_khr_fp64")) {
      BOOST_TEST_MESSAGE(
          "Test disabled, device (" << device.name()
                                    << ") does not support cl_khr_fp64, i.e., "
                                       "double precision floating point");
      return;
    }
  }

  boost::compute::context context(device);
  boost::compute::command_queue queue(context, device);

  unsigned int seed = std::random_device()();
  BOOST_TEST_MESSAGE("SEED = " << seed);
  auto generator = create_random_generator<scalar_type>(seed);

  std::vector<value_type> asrc;
  jb::testing::create_random_timeseries(generator, size, asrc);

  boost::compute::vector<value_type> a(size, context);

  boost::compute::copy(asrc.begin(), asrc.begin() + size, a.begin(), queue);
  std::vector<value_type> acpy(size);
  boost::compute::copy(a.begin(), a.begin() + size, acpy.begin(), queue);
  for (std::size_t i = 0; i != acpy.size(); ++i) {
    JB_LOG(trace) << "    " << i << " " << acpy[i] << " " << asrc[i];
  }

  reduce_sum<value_type> reducer(size, queue);
  auto done = reducer.execute(a.begin(), a.begin() + subset_size);
  done.wait();

  value_type expected =
      std::accumulate(asrc.begin(), asrc.begin() + subset_size, value_type(0));
  value_type actual = *done.get();
  BOOST_CHECK_MESSAGE(
      jb::testing::check_close_enough(actual, expected, size),
      "mismatched C++ vs. OpenCL results expected(C++)="
          << expected << " actual(OpenCL)=" << actual
          << " delta=" << (actual - expected));
}

template <typename value_type>
void check_generic_reduce(std::size_t size) {
  check_generic_reduce_sized<value_type>(size, size);
}

} // anonymous namespace

/**
 * @test Make sure the jb::tde::generic_reduce() works as
 * expected.for sizes around ~256, which is close to
 * MAX_WORK_GROUP_SIZE
 */
BOOST_AUTO_TEST_CASE(generic_reduce_int_2e6) {
  int const N = 16;
  for (int i = -N / 2; i != N / 2; ++i) {
    std::size_t const size = (1 << 8) + i;
    check_generic_reduce<int>(size);
  }
}

/**
 * @test Make sure the jb::tde::generic_reduce() works as
 * expected.for sizes around ~256 * 32, which is close to
 * MAX_WORK_GROUP_SIZE * MAX_COMPUTE_UNITS
 */
BOOST_AUTO_TEST_CASE(generic_reduce_int_2e13) {
  int const N = 16;
  for (int i = -N / 2; i != N / 2; ++i) {
    std::size_t const size = (1 << 13) + i;
    check_generic_reduce<int>(size);
  }
}

/**
 * @test Make sure the jb::tde::generic_reduce() works as
 * expected for 2^20 (1 million binary)
 */
BOOST_AUTO_TEST_CASE(generic_reduce_int_2e20) {
  std::size_t const size = (1 << 20);
  check_generic_reduce<int>(size);
}

/**
 * @test Make sure the jb::tde::generic_reduce() works as
 * expected for 1000000
 */
BOOST_AUTO_TEST_CASE(generic_reduce_int_1000000) {
  std::size_t const size = 1000 * 1000;
  check_generic_reduce<int>(size);
}

/**
 * @test Make sure the jb::tde::generic_reduce() works as
 * expected for a number that does not have a lot of powers of 2.
 */
BOOST_AUTO_TEST_CASE(generic_reduce_int_PRIMES) {
  std::size_t const size = 2 * 3 * 5 * 7 * 11 * 13 * 17 * 19;
  check_generic_reduce<int>(size);
}

/**
 * @test Make sure the jb::tde::generic_reduce() works as
 * expected for a number without a lot of powers of 2
 */
BOOST_AUTO_TEST_CASE(generic_reduce_float_PRIMES) {
  std::size_t const size = 2 * 3 * 5 * 7 * 11 * 13 * 17;
  check_generic_reduce<float>(size);
}

/**
 * @test Make sure the jb::tde::generic_reduce() works as
 * expected for a number without a lot of powers of 2
 */
BOOST_AUTO_TEST_CASE(generic_reduce_complex_float_PRIMES) {
  std::size_t const size = 2 * 3 * 5 * 7 * 11 * 13;
  check_generic_reduce<std::complex<float>>(size);
}

/**
 * @test Make sure the jb::tde::generic_reduce() works as
 * expected for a number without a lot of powers of 2
 */
BOOST_AUTO_TEST_CASE(generic_reduce_complex_double_PRIMES) {
  std::size_t const size = 2 * 3 * 5 * 7 * 11 * 13;
  check_generic_reduce<std::complex<double>>(size);
}

/**
 * @test Make sure the jb::tde::generic_reduce() works as
 * expected when used with a subset of the input.
 */
BOOST_AUTO_TEST_CASE(generic_reduce_double_subset) {
  std::size_t const size = 1000000;
  std::size_t const subset_size = size / 2;
  check_generic_reduce_sized<double>(size, subset_size);
}
