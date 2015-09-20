#include <jb/opencl/config.hpp>
#include <jb/opencl/device_selector.hpp>
#include <jb/opencl/copy_to_host_async.hpp>
#include <jb/testing/microbenchmark.hpp>

#include <boost/compute/algorithm/copy.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/container/vector.hpp>
#include <iostream>
#include <string>
#include <stdexcept>

namespace {

class config : public jb::config_object {
 public:
  config()
      : benchmark(desc("benchmark"), this)
      , opencl(desc("opencl"), this)
  {}

  jb::config_attribute<config, jb::testing::microbenchmark_config> benchmark;
  jb::config_attribute<config, jb::opencl::config> opencl;
};

int nsamples = 2048;

template<bool pipelined>
class fixture {
 public:
  fixture(
      boost::compute::context& context,
      boost::compute::command_queue& q)
      : fixture(nsamples, context, q)
  {}
  fixture(
      int size,
      boost::compute::context& context,
      boost::compute::command_queue& q)
      : src(size)
      , dev(size, context)
      , dst(size)
      , queue(q)
  {}

  void run() {
    auto upload_done = boost::compute::copy_async(
        src.begin(), src.end(), dev.begin(), queue);
    auto download_done = jb::opencl::copy_to_host_async(
        dev.begin(), dev.end(), dst.begin(), queue,
        boost::compute::wait_list(upload_done.get_event()));
    download_done.wait();
  }

 private:
  std::vector<std::complex<float>> src;
  boost::compute::vector<std::complex<float>> dev;
  std::vector<std::complex<float>> dst;
  boost::compute::command_queue queue;
};

template<>
void fixture<false>::run() {
  boost::compute::copy(src.begin(), src.end(), dev.begin(), queue);
  boost::compute::copy(dev.begin(), dev.end(), dst.begin(), queue);
}

template<bool pipelined>
void benchmark_test_case(
    config const& cfg) {
  boost::compute::device device = jb::opencl::device_selector(cfg.opencl());
  boost::compute::context context(device);
  boost::compute::command_queue queue(context, device);
  typedef jb::testing::microbenchmark<fixture<pipelined>> benchmark;
  benchmark bm(cfg.benchmark());

  auto r = bm.run(context, queue);
  typename benchmark::summary s(r);
  std::cerr << cfg.benchmark().test_case() << " summary " << s << std::endl;
  if (cfg.benchmark().verbose()) {
    bm.write_results(std::cout, r);
  }
}

} // anonymous namespace

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.benchmark(
      jb::testing::microbenchmark_config().test_case("complex:float:async"))
      .process_cmdline(argc, argv);

  std::cerr << "Configuration for test\n" << cfg << std::endl;

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
       << ", complex:float:sync"
       << std::endl;
    throw jb::usage(os.str(), 1);
  }

  return 0;
} catch(jb::usage const& ex) {
  std::cerr << "usage: " << ex.what() << std::endl;
  return ex.exit_status();
} catch(std::exception const& ex) {
  std::cerr << "standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch(...) {
  std::cerr << "unknown exception raised" << std::endl;
  return 1;
}

