#include <jb/clfft/plan.hpp>
#include <jb/opencl/config.hpp>
#include <jb/opencl/copy_to_host_async.hpp>
#include <jb/opencl/device_selector.hpp>
#include <jb/testing/microbenchmark.hpp>

#include <boost/compute/command_queue.hpp>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

class config : public jb::config_object {
public:
  config()
      : benchmark(desc("benchmark"), this)
      , opencl(desc("opencl"), this) {
  }

  jb::config_attribute<config, jb::testing::microbenchmark_config> benchmark;
  jb::config_attribute<config, jb::opencl::config> opencl;
};

int nsamples = 2048;

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
  auto download_done = jb::opencl::copy_to_host_async(
      out.begin(), out.end(), dst.begin(), queue, wait_list(fft_done));
  download_done.wait();
  return static_cast<int>(src.size());
}

template <bool pipelined>
void benchmark_test_case(config const& cfg) {
  jb::clfft::init init;
  boost::compute::device device = jb::opencl::device_selector(cfg.opencl());
  boost::compute::context context(device);
  boost::compute::command_queue queue(context, device);
  typedef jb::testing::microbenchmark<fixture<pipelined>> benchmark;
  benchmark bm(cfg.benchmark());

  auto r = bm.run(context, queue);
  bm.typical_output(r);
}

} // anonymous namespace

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.benchmark(
         jb::testing::microbenchmark_config().test_case("complex:float:async"))
      .process_cmdline(argc, argv);

  std::cout << "Configuration for test\n" << cfg << std::endl;

  auto test_case = cfg.benchmark().test_case();
  if (test_case == "complex:float:async") {
    benchmark_test_case<true>(cfg);
  } else if (test_case == "complex:float:sync") {
    benchmark_test_case<false>(cfg);
  } else {
    std::ostringstream os;
    os << "Unknown test case (" << test_case << ")" << std::endl;
    os << " --test-case must be one of"
       << ": complex:float:async"
       << ", complex:float:sync" << std::endl;
    throw jb::usage(os.str(), 1);
  }

  return 0;
} catch (jb::usage const& ex) {
  std::cerr << "usage: " << ex.what() << std::endl;
  return ex.exit_status();
} catch (std::exception const& ex) {
  std::cerr << "standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "unknown exception raised" << std::endl;
  return 1;
}
