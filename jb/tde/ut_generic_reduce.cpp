#include <jb/tde/generic_reduce_program.hpp>
#include <jb/opencl/copy_to_host_async.hpp>
#include <jb/opencl/config.hpp>
#include <jb/opencl/device_selector.hpp>
#include <jb/complex_traits.hpp>
#include <jb/log.hpp>
#include <jb/p2ceil.hpp>
#include <jb/testing/check_complex_close_enough.hpp>
#include <jb/testing/create_random_timeseries.hpp>

#include <boost/compute/container.hpp>
#include <boost/compute/context.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/type_traits.hpp>
#include <boost/compute/types/complex.hpp>
#include <boost/test/unit_test.hpp>
#include <memory>
#include <random>
#include <sstream>

namespace jb {
namespace tde {

template<typename T>
class reduce_sum {
 public:
  reduce_sum(std::size_t size, boost::compute::command_queue const& queue)
      : size_(size)
      , queue_(queue)
      , program_(create_program(queue))
      , initial_(program_, "generic_transform_reduce_initial")
      , intermediate_(program_, "generic_transform_reduce_intermediate")
  {
    // ... size the first pass of the reduction.  We need to balance two
    // constraints:
    //   (a) We cannot use more memory than whatever the device
    //   supports, since the algorithm uses one entry in scratch per
    //   local thread that could limit the number of local threads.
    //   This is extremely unlikely, but who wants crashes?
    //   (b) The maximum size of a workgroup limits by how much we can
    //   reduce in a single pass, so smaller workgroups require more
    //   passes and more intermediate memory.
    // first we query the device ...
    boost::compute::device device = queue_.get_device();
    std::size_t local_mem_size = device.local_memory_size();
    max_workgroup_size_ = device.max_work_group_size();

    // ... the we convert the bytes into array sizes ...
    boost::compute::kernel sizer(program_, "scratch_element_size");
    boost::compute::vector<boost::compute::ulong_> dev(1, queue_.get_context());
    sizer.set_arg(0, dev);
    queue_.enqueue_1d_range_kernel(sizer, 0, 1, 1).wait();
    std::vector<boost::compute::ulong_> host(1);
    boost::compute::copy(dev.begin(), dev.end(), host.begin(), queue_);
    sizeof_output_type_ = host[0];
    scratch_size_ = local_mem_size / sizeof_output_type_;
    // ... this is the largest amount that we might need from local
    // scratch ...
    scratch_size_ = std::min(scratch_size_, max_workgroup_size_);

    // ... now on to compute the reduction factor, first how many
    // threads can we effectively use, unless local scratch is tiny,
    // almost all of the time this would be max_workgroup_size ...
    effective_workgroup_size_ = std::min(scratch_size_, max_workgroup_size_);

    // ... so in a single pass we will need this many workgroups to
    // handle all the data ...
    auto workgroups = std::max(
        std::size_t(1), size_ / effective_workgroup_size_);

    JB_LOG(trace)
        << "Reducer plan settings:"
        << "\n  size_ = " << size_
        << "\n  sizeof(output)=" << sizeof_output_type_
        << "\n  scratch_size=" << scratch_size_
        << "\n  local_mem_size=" << local_mem_size
        << "\n  max_workgroup_size=" << max_workgroup_size_
        << "\n  effective_workgroup_size=" << effective_workgroup_size_
        << "\n  workgroups=" << workgroups
        << "\n  workgroups * effective_workgroup_size="
        << workgroups * effective_workgroup_size_;

    // ... a lot of that silly math was to size the output buffer ...
    ping_ = boost::compute::vector<T>(workgroups, queue_.get_context());
    pong_ = boost::compute::vector<T>(workgroups, queue_.get_context());
  }

  typedef typename boost::compute::vector<T>::iterator vector_iterator;
  boost::compute::future<vector_iterator> execute(
      std::vector<T> const& orig,
      boost::compute::vector<T> const& src,
      boost::compute::wait_list const& wait = boost::compute::wait_list()) {
    if (src.size() != size_) {
      throw std::invalid_argument("mismatched size");
    }
    
    auto workgroup_size = effective_workgroup_size_;
    auto workgroups = size_ / workgroup_size;
    if (workgroups == 0) {
      workgroups = 1;
    }
    auto div = std::div(
        static_cast<long long>(size_),
        static_cast<long long>(workgroups * workgroup_size));
    auto VPT = div.quot + (div.rem != 0);

    JB_LOG(trace)
        << "Executing (initial) reducer plan for :"
        << "\n    size_ = " << size_
        << "\n    sizeof(output)=" << sizeof_output_type_
        << "\n    scratch_size=" << scratch_size_
        << "\n    max_workgroup_size=" << max_workgroup_size_
        << "\n    effective_workgroup_size=" << effective_workgroup_size_
        << "\n    workgroups=" << workgroups
        << "\n    workgroup_size=" << workgroup_size
        << "\n    worgroups*workgroup_size="
        << workgroups * workgroup_size
        << "\n    arg.VPT=" << VPT
        << "\n    arg.TPB=" << workgroup_size
        << "\n    arg.N=" << size_
        ;

    int arg = 0;
    initial_.set_arg(arg++, ping_);
    initial_.set_arg(arg++, boost::compute::ulong_(VPT));
    initial_.set_arg(arg++, boost::compute::ulong_(workgroup_size));
    initial_.set_arg(arg++, boost::compute::ulong_(size_));
    initial_.set_arg(arg++, src);
    initial_.set_arg(
        arg++, boost::compute::local_buffer<T>(workgroup_size));
    auto event = queue_.enqueue_1d_range_kernel(
        initial_, 0, workgroups * workgroup_size, workgroup_size, wait);

    for (auto pass_output_size = workgroups;
         pass_output_size > 1;
         pass_output_size = workgroups) {
      // ... it is possible (specially towards the end) that we do not
      // have enough work to fill a "workgroup_size" number of
      // local work items, in that case, just limit the local size even
      // further ...
      if (pass_output_size < workgroup_size) {
        workgroup_size = jb::p2ceil(pass_output_size) / 2;
      }
      workgroups = pass_output_size / workgroup_size;
      if (workgroups == 0) {
        workgroups = 1;
      }
      auto div = std::div(
          static_cast<long long>(pass_output_size),
          static_cast<long long>(workgroups * workgroup_size));
      auto VPT = div.quot + (div.rem != 0);

      JB_LOG(trace)
          << "  executing (intermediate) reducer plan with :"
          << "\n    size_ = " << size_
          << "\n    sizeof(output)=" << sizeof_output_type_
          << "\n    scratch_size=" << scratch_size_
          << "\n    max_workgroup_size=" << max_workgroup_size_
          << "\n    effective_workgroup_size=" << effective_workgroup_size_
          << "\n    workgroups=" << workgroups
          << "\n    workgroup_size=" << workgroup_size
          << "\n    worgroups*workgroup_size=" << workgroups * workgroup_size
          << "\n    pass_output_size=" << pass_output_size
          << "\n    arg.VPT=" << VPT
          << "\n    arg.TPB=" << workgroup_size
          << "\n    arg.N=" << pass_output_size
          ;

      // ... prepare the kernel ...
      int arg = 0;
      intermediate_.set_arg(arg++, pong_);
      intermediate_.set_arg(arg++, boost::compute::ulong_(VPT));
      intermediate_.set_arg(arg++, boost::compute::ulong_(workgroup_size) /* TPB */);
      intermediate_.set_arg(arg++, boost::compute::ulong_(pass_output_size));
      intermediate_.set_arg(arg++, ping_);
      intermediate_.set_arg(
          arg++, boost::compute::local_buffer<T>(workgroup_size));
      event = queue_.enqueue_1d_range_kernel(
          intermediate_, 0, workgroups * workgroup_size, workgroup_size,
          boost::compute::wait_list(event));

      std::swap(ping_, pong_);
    }
    return boost::compute::make_future(ping_.begin(), event);
  }

 private:
  boost::compute::program create_program(
      boost::compute::command_queue const& queue) {
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
    JB_LOG(trace)
        << "================ cut here ================\n"
        << os.str() << "\n"
        << "================ cut here ================\n"
        ;
    auto program = boost::compute::program::create_with_source(
        os.str(), queue.get_context());
    try {
      program.build();
    } catch(boost::compute::opencl_error const& ex) {
      JB_LOG(error) << "errors building program: "
                    << ex.what() << "\n"
                    << program.build_log() << "\n";
      throw;
    }
    return program;
  }
    

 private:
  std::size_t size_;
  boost::compute::command_queue queue_;
  boost::compute::program program_;
  boost::compute::kernel initial_;
  boost::compute::kernel intermediate_;
  std::size_t max_workgroup_size_;
  std::size_t sizeof_output_type_;
  std::size_t scratch_size_;
  std::size_t effective_workgroup_size_;
  boost::compute::vector<T> ping_;
  boost::compute::vector<T> pong_;
};


} // namespace tde
} // namespace jb

namespace {

template<typename T>
std::function<T()> create_random_generator(unsigned int seed) {
  std::mt19937 gen(seed);
  std::uniform_int_distribution<T> dis(-1000, 1000);
  typedef std::pair<std::mt19937, std::uniform_int_distribution<T>> state_type;
  std::shared_ptr<state_type> state(new state_type(gen, dis));
  return [state]() { return state->second(state->first); };
}

template<>
std::function<float()> create_random_generator<float>(unsigned int seed) {
  std::mt19937 gen(seed);
  std::uniform_real_distribution<float> dis(1, 2);
  typedef std::pair<std::mt19937, std::uniform_real_distribution<float>> state_type;
  std::shared_ptr<state_type> state(new state_type(gen, dis));
  return [state]() { return state->second(state->first); };
}

template<>
std::function<double()> create_random_generator<double>(unsigned int seed) {
  std::mt19937 gen(seed);
  std::uniform_real_distribution<double> dis(1, 2);
  typedef std::pair<std::mt19937, std::uniform_real_distribution<double>> state_type;
  std::shared_ptr<state_type> state(new state_type(gen, dis));
  return [state]() { return state->second(state->first); };
}

template<typename value_type>
void check_generic_reduce(std::size_t size) {
  BOOST_MESSAGE("Testing with size = " << size);
  boost::compute::device device = jb::opencl::device_selector(jb::opencl::config());
  BOOST_MESSAGE("Running on device = " << device.name());
  boost::compute::context context(device);
  boost::compute::command_queue queue(context, device);

  unsigned int seed = std::random_device()();
  BOOST_MESSAGE("SEED = " << seed);
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

  value_type expected = std::accumulate(
      asrc.begin(), asrc.end(), value_type(0));
  value_type actual = *done.get();
  BOOST_CHECK_MESSAGE(
      jb::testing::close_enough(actual, expected, size),
      "mismatched CPU vs. GPU results expected(CPU)=" << expected
      << " actual(GPU)=" << actual
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
  for (int i = -N/2; i != N/2; ++i) {
    std::size_t const size = (1<<8) + i;
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
  for (int i = -N/2; i != N/2; ++i) {
    std::size_t const size = (1<<13) + i;
    check_generic_reduce<int>(size);
  }
}

/**
 * @test Make sure the jb::tde::generic_reduce() works as
 * expected for 2^20 (1 million binary)
 */
BOOST_AUTO_TEST_CASE(generic_reduce_int_2e20) {
  std::size_t const size = (1<<20);
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
  BOOST_MESSAGE("max_mem=" << max_mem << " sizeof(int)=" << sizeof(int));
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
