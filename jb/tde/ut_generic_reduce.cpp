#include <jb/tde/generic_reduce_program.hpp>
#include <jb/opencl/device_selector.hpp>
#include <jb/opencl/copy_to_host_async.hpp>
#include <jb/log.hpp>
#include <jb/testing/check_vector_close_enough.hpp>
#include <jb/testing/create_random_timeseries.hpp>

#include <boost/compute/container.hpp>
#include <boost/compute/context.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/types/complex.hpp>
#include <boost/test/unit_test.hpp>
#include <random>
#include <sstream>

namespace jb {
namespace tde {

class reduce_sum {
 public:
  reduce_sum(boost::compute::command_queue const& queue) {
    std::ostringstream os;
    os << R"""(
void reduce_initialize(int* lhs) {
  *lhs = 0;
}
void reduce_transform(
    int* accumulated, __global int const* value, int offset) {
  *accumulated = value[offset];
}
void reduce_combine(int* accumulated, int* value) {
  *accumulated = *accumulated + *value;
}

)""";
    os << generic_reduce_program_source;
    std::ostringstream flags;
    flags << "-DTPB=8"
          << " -DVPT=128"
          << " -DREDUCE_TYPENAME_INPUT=int"
          << " -DREDUCE_TYPENAME_OUTPUT=int";
    auto program = boost::compute::program::create_with_source(
        os.str(), queue.get_context());
    try {
      program.build(flags.str());
    } catch(boost::compute::opencl_error const& ex) {
      JB_LOG(error) << "errors building program: "
                    << ex.what() << "\n"
                    << program.build_log() << "\n";
      throw;
    }
  }

  boost::compute::future<int> execute(
      boost::compute::vector<int> const& src,
      boost::compute::wait_list const& wait = boost::compute::wait_list()) {
    int x = 0;
    return boost::compute::make_future(x, boost::compute::event());
  }

 private:
  boost::compute::command_queue queue_;
  boost::compute::program program_;
  boost::compute::kernel reduce_initial_;
  boost::compute::kernel reduce_intermediate_;
};


} // namespace tde
} // namespace jb

namespace {

template<typename value_type>
void check_generic_reduce(std::size_t size) {
  boost::compute::device device = jb::opencl::device_selector();
  boost::compute::context context(device);
  boost::compute::command_queue queue(context, device);

  unsigned int seed = std::random_device()();
  std::mt19937 gen(seed);
  std::uniform_int_distribution<value_type> dis(-1000, 1000);
  auto generator = [&gen, &dis]() { return dis(gen); };
  BOOST_MESSAGE("SEED = " << seed);

  std::vector<value_type> asrc;
  jb::testing::create_random_timeseries(generator, size, asrc);

  boost::compute::vector<value_type> a(size, context);

  boost::compute::copy(asrc.begin(), asrc.end(), a.begin(), queue);

  jb::tde::reduce_sum reducer(queue);
  auto done = reducer.execute(a);
  done.wait();

  auto expected = std::accumulate(asrc.begin(), asrc.end(), 0);
  auto actual = done.get();
  BOOST_CHECK_EQUAL(expected, actual);
}

} // anonymous namespace

/**
 * @test Make sure the jb::tde::generic_reduce() works as
 * expected.for sizes around ~1024
 */
BOOST_AUTO_TEST_CASE(generic_reduce_int_2e4) {
  int const N = 1024;
  for (int i = 0; i != N; ++i) {
    std::size_t const size = i + (1<<4);
    BOOST_MESSAGE("Testing with size = " << size);
    check_generic_reduce<int>(size);
  }
}

