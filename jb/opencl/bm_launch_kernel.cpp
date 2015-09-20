#include <jb/opencl/build_simple_kernel.hpp>
#include <jb/opencl/config.hpp>
#include <jb/opencl/device_selector.hpp>
#include <jb/testing/microbenchmark.hpp>

#include <boost/compute/algorithm/copy.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/utility/wait_list.hpp>
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

char const source[] = R"""(
__kernel void empty() {
}
)""";

class fixture {
 public:
  fixture(
      boost::compute::context& context,
      boost::compute::command_queue& q)
      : fixture(1, context, q)
  {}
  fixture(
      int size,
      boost::compute::context& context,
      boost::compute::command_queue& q)
      : chain_length(size)
      , kernel(jb::opencl::build_simple_kernel(
          context, context.get_device(), source, "empty"))
      , queue(q)
  {}

  void run() {
    boost::compute::wait_list wait;
    for (int i = 0; i != chain_length; ++i) {
      auto event = queue.enqueue_task(kernel, wait);
      wait = boost::compute::wait_list(event);
    }
    wait.wait();
  }

 private:
  int chain_length;
  boost::compute::kernel kernel;
  boost::compute::command_queue queue;
};

void benchmark_test_case(config const& cfg) {
  boost::compute::device device = jb::opencl::device_selector(cfg.opencl());
  boost::compute::context context(device);
  boost::compute::command_queue queue(context, device);
  typedef jb::testing::microbenchmark<fixture> benchmark;
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
  cfg.process_cmdline(argc, argv);
  std::cerr << "Configuration for test\n" << cfg << std::endl;

  benchmark_test_case(cfg);

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

