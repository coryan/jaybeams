#include <jb/clfft/plan.hpp>
#include <jb/opencl/device_selector.hpp>
#include <jb/opencl/microbenchmark_config.hpp>
#include <jb/testing/microbenchmark.hpp>
#include <jb/testing/microbenchmark_group_main.hpp>

#include <boost/compute/command_queue.hpp>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>

/// Helper types and functions for (trivial) benchmarks of clFFT.
namespace {
using config = jb::opencl::microbenchmark_config;
jb::testing::microbenchmark_group<config> create_testcases();
} // anonymous namespace

int main(int argc, char* argv[]) {
  auto testcases = create_testcases();
  return jb::testing::microbenchmark_group_main(argc, argv, testcases);
}

namespace {
// By default test with around one million samples
int nsamples = 1 << 20;

/**
 * The fixture for this benchmark.
 *
 * @tparam pipelined if true, the copy operations are pipelined with
 * the computation operations.
 */
template <bool pipelined>
class fixture {
public:
  fixture(boost::compute::context& context, boost::compute::command_queue& q)
      : fixture(nsamples, context, q) {
  }
  fixture(
      int size, boost::compute::context& context,
      boost::compute::command_queue& q)
      : src(size)
      , in(size, context)
      , out(size, context)
      , dst(size)
      , queue(q)
      , fft(jb::clfft::create_forward_plan_1d(out, in, context, queue)) {
  }

  int run() {
    boost::compute::copy(src.begin(), src.end(), in.begin(), queue);
    fft.enqueue(out, in, queue).wait();
    boost::compute::copy(out.begin(), out.end(), dst.begin(), queue);
    return static_cast<int>(src.size());
  }

private:
  typedef boost::compute::vector<std::complex<float>> invector;
  typedef boost::compute::vector<std::complex<float>> outvector;
  std::vector<std::complex<float>> src;
  invector in;
  outvector out;
  std::vector<std::complex<float>> dst;
  boost::compute::command_queue queue;
  jb::clfft::plan<invector, outvector> fft;
};

template <>
int fixture<true>::run() {
  using namespace boost::compute;
  auto upload_done = copy_async(src.begin(), src.end(), in.begin(), queue);
  auto fft_done =
      fft.enqueue(out, in, queue, wait_list(upload_done.get_event()));
  queue.enqueue_barrier();
  auto download_done = copy_async(out.begin(), out.end(), dst.begin(), queue);
  download_done.wait();
  return static_cast<int>(src.size());
}

template <bool pipelined>
std::function<void(config const&)> test_case() {
  return [](config const& cfg) {
    jb::clfft::init init;
    boost::compute::device device = jb::opencl::device_selector(cfg.opencl());
    boost::compute::context context(device);
    boost::compute::command_queue queue(context, device);
    typedef jb::testing::microbenchmark<fixture<pipelined>> benchmark;
    benchmark bm(cfg.microbenchmark());

    auto r = bm.run(context, queue);
    bm.typical_output(r);
  };
}

jb::testing::microbenchmark_group<config> create_testcases() {
  return jb::testing::microbenchmark_group<config>{
      {"complex:float:async", test_case<true>()},
      {"complex:float:sync", test_case<false>()},
  };
}

} // anonymous namespace
