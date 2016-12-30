#include <jb/opencl/config.hpp>
#include <jb/opencl/copy_to_host_async.hpp>
#include <jb/opencl/device_selector.hpp>
#include <jb/tde/generic_reduce.hpp>
#include <jb/testing/check_complex_close_enough.hpp>
#include <jb/testing/create_random_timeseries.hpp>
#include <jb/complex_traits.hpp>
#include <jb/log.hpp>
#include <jb/p2ceil.hpp>

#include <boost/compute/command_queue.hpp>
#include <boost/compute/container.hpp>
#include <boost/compute/context.hpp>
#include <boost/compute/type_traits.hpp>
#include <boost/compute/types/complex.hpp>
#include <boost/test/unit_test.hpp>
#include <memory>
#include <random>
#include <sstream>

namespace jb {
namespace tde {

template <typename T>
class reduce_sum : public generic_reduce<reduce_sum<T>, T, T> {
public:
  reduce_sum(std::size_t size, boost::compute::command_queue const& queue)
      : generic_reduce<reduce_sum<T>, T, T>(size, queue) {
  }

  static boost::compute::program
  create_program(boost::compute::command_queue const& queue) {
    std::ostringstream os;
    os << "typedef " << boost::compute::type_name<T>() << " reduce_input_t;\n";
    os << "typedef " << boost::compute::type_name<T>() << " reduce_output_t;\n";
    os << R"""(
inline void reduce_initialize(reduce_output_t* lhs) {
  *lhs = (reduce_output_t)(0);
}
inline void reduce_transform(
    reduce_output_t* lhs, reduce_input_t const* value) {
  *lhs = *value;
}
inline void reduce_combine(
    reduce_output_t* accumulated, reduce_output_t* value) {
  *accumulated = *accumulated + *value;
}

)""";
    os << generic_reduce_program_source;
    JB_LOG(trace) << "================ cut here ================\n"
                  << os.str() << "\n"
                  << "================ cut here ================\n";
    auto program = boost::compute::program::create_with_source(
        os.str(), queue.get_context());
    try {
      program.build();
    } catch (boost::compute::opencl_error const& ex) {
      JB_LOG(error) << "errors building program: " << ex.what() << "\n"
                    << program.build_log() << "\n";
      throw;
    }
    return program;
  }
};

} // namespace tde
} // namespace jb

namespace {

template <typename T>
std::function<T()> create_random_generator(unsigned int seed) {
  std::mt19937 gen(seed);
  std::uniform_int_distribution<T> dis(-1000, 1000);
  typedef std::pair<std::mt19937, std::uniform_int_distribution<T>> state_type;
  std::shared_ptr<state_type> state(new state_type(gen, dis));
  return [state]() { return state->second(state->first); };
}

template <>
std::function<float()> create_random_generator<float>(unsigned int seed) {
  std::mt19937 gen(seed);
  std::uniform_real_distribution<float> dis(1, 2);
  typedef std::pair<std::mt19937, std::uniform_real_distribution<float>>
      state_type;
  std::shared_ptr<state_type> state(new state_type(gen, dis));
  return [state]() { return state->second(state->first); };
}

template <>
std::function<double()> create_random_generator<double>(unsigned int seed) {
  std::mt19937 gen(seed);
  std::uniform_real_distribution<double> dis(1, 2);
  typedef std::pair<std::mt19937, std::uniform_real_distribution<double>>
      state_type;
  std::shared_ptr<state_type> state(new state_type(gen, dis));
  return [state]() { return state->second(state->first); };
}

template <typename value_type>
void check_generic_reduce(std::size_t size) {
  BOOST_TEST_MESSAGE("Testing with size = " << size);
  boost::compute::device device =
      jb::opencl::device_selector(jb::opencl::config());
  BOOST_TEST_MESSAGE("Running on device = " << device.name());
  boost::compute::context context(device);
  boost::compute::command_queue queue(context, device);

  unsigned int seed = std::random_device()();
  BOOST_TEST_MESSAGE("SEED = " << seed);
  typedef typename jb::extract_value_type<value_type>::precision scalar_type;
  auto generator = create_random_generator<scalar_type>(seed);

  std::vector<value_type> asrc;
  jb::testing::create_random_timeseries(generator, size, asrc);

  boost::compute::vector<value_type> a(size, context);

  boost::compute::copy(asrc.begin(), asrc.end(), a.begin(), queue);
  std::vector<value_type> acpy(size);
  boost::compute::copy(a.begin(), a.end(), acpy.begin(), queue);
  for (std::size_t i = 0; i != acpy.size(); ++i) {
    JB_LOG(trace) << "    " << i << " " << acpy[i] << " " << asrc[i];
  }

  jb::tde::reduce_sum<value_type> reducer(size, queue);
  auto done = reducer.execute(asrc, a);
  done.wait();

  value_type expected =
      std::accumulate(asrc.begin(), asrc.end(), value_type(0));
  value_type actual = *done.get();
  BOOST_CHECK_MESSAGE(
      jb::testing::close_enough(actual, expected, size),
      "mismatched CPU vs. GPU results expected(CPU)="
          << expected << " actual(GPU)=" << actual
          << " delta=" << (actual - expected));
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
 * expected for 1000000
 */
BOOST_AUTO_TEST_CASE(generic_reduce_int_PRIMES) {
  std::size_t const size = 2 * 3 * 5 * 7 * 11 * 13 * 17 * 19;
  check_generic_reduce<int>(size);
}

#if 0
/**
 * @test Make sure the jb::tde::generic_reduce() works as
 * expected for something around a billion
 */
BOOST_AUTO_TEST_CASE(generic_reduce_int_MAX) {
  boost::compute::device device = jb::opencl::device_selector();
  auto max_mem = device.max_memory_alloc_size();
  std::size_t const size = max_mem / sizeof(int);
  BOOST_TEST_MESSAGE("max_mem=" << max_mem << " sizeof(int)=" << sizeof(int));
  check_generic_reduce<int>(size);
}
#endif /* 0 */

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
