#include <jb/tde/generic_reduce_program.hpp>
#include <jb/opencl/device_selector.hpp>
#include <jb/opencl/copy_to_host_async.hpp>
#include <jb/p2ceil.hpp>
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
  reduce_sum(std::size_t size, boost::compute::command_queue const& queue)
      : size_(size)
      , queue_(queue)
      , program_(create_program(queue))
      , initial_(program_, "generic_transform_reduce_initial")
      , intermediate_(program_, "generic_transform_reduce_initial")
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
        std::size_t(1), size / effective_workgroup_size_);

    BOOST_MESSAGE("Reducer plan settings:"
                  << "\n  sizeof(output)=" << sizeof_output_type_
                  << "\n  scratch_size=" << scratch_size_
                  << "\n  local_mem_size=" << local_mem_size
                  << "\n  max_workgroup_size=" << max_workgroup_size_
                  << "\n  effective_workgroup_size=" << effective_workgroup_size_
                  << "\n  workgroups=" << workgroups);

    // ... a lot of that silly math was to size the output buffer ...
    ping_ = boost::compute::vector<int>(workgroups, queue_.get_context());
    pong_ = boost::compute::vector<int>(workgroups, queue_.get_context());
  }

  boost::compute::future<boost::compute::vector<int>::iterator> execute(
      boost::compute::vector<int> const& src,
      boost::compute::wait_list const& wait = boost::compute::wait_list()) {
    if (src.size() != size_) {
      throw std::invalid_argument("mismatched size");
    }
    
    auto workgroup_size = effective_workgroup_size_;
    auto workgroups = size_ / workgroup_size;
    if (workgroups == 0) {
      workgroups = 1;
    }

    std::size_t local_work_size = max_workgroup_size_;
    std::size_t global_work_size =
        (src.size() / local_work_size) * local_work_size;
    if (global_work_size < src.size()) {
      global_work_size += local_work_size;
    }
    auto div = std::div(
        static_cast<long long>(size_),
        static_cast<long long>(workgroups * workgroup_size));
    auto VPT = div.quot + (div.rem != 0);
    
    int arg = 0;
    initial_.set_arg(arg++, ping_);
    initial_.set_arg(arg++, boost::compute::ulong_(VPT));
    initial_.set_arg(arg++, boost::compute::ulong_(workgroup_size));
    initial_.set_arg(
        arg++, boost::compute::local_buffer<int>(workgroup_size));
    initial_.set_arg(arg++, boost::compute::ulong_(size_));
    initial_.set_arg(arg++, src);
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
      VPT = 1;

      // ... prepare the kernel ...
      int arg = 0;
      intermediate_.set_arg(arg++, pong_);
      intermediate_.set_arg(arg++, boost::compute::ulong_(VPT));
      intermediate_.set_arg(arg++, boost::compute::ulong_(workgroup_size) /* TPB */);
      intermediate_.set_arg(
          arg++, boost::compute::local_buffer<int>(workgroup_size));
      intermediate_.set_arg(arg++, boost::compute::ulong_(pass_output_size));
      intermediate_.set_arg(arg++, ping_);
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
    flags << "-DREDUCE_TYPENAME_INPUT=int"
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
  boost::compute::vector<int> ping_;
  boost::compute::vector<int> pong_;
};


} // namespace tde
} // namespace jb

namespace {

template<typename value_type>
void check_generic_reduce(std::size_t size) {
  BOOST_MESSAGE("Testing with size = " << size);
  boost::compute::device device = jb::opencl::device_selector();
  BOOST_MESSAGE("Running on device = " << device.name());
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

  jb::tde::reduce_sum reducer(size, queue);
  auto done = reducer.execute(a);
  done.wait();

  int expected = std::accumulate(asrc.begin(), asrc.end(), 0);
  int actual = *done.get();
  BOOST_CHECK_EQUAL(expected, actual);
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
