#include <jb/opencl/build_simple_kernel.hpp>
#include <jb/opencl/device_selector.hpp>
#include <jb/opencl/microbenchmark_config.hpp>
#include <jb/testing/microbenchmark.hpp>

#include <boost/compute/algorithm/copy.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/utility/wait_list.hpp>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using config = jb::opencl::microbenchmark_config;

char const source[] = R"""(
__kernel void empty() {
}
)""";

class fixture {
public:
  fixture(boost::compute::context& context, boost::compute::command_queue& q)
      : fixture(1, context, q) {
  }
  fixture(
      int size, boost::compute::context& context,
      boost::compute::command_queue& q)
      : chain_length(size)
      , kernel(jb::opencl::build_simple_kernel(
            context, context.get_device(), source, "empty"))
      , queue(q) {
  }

  int run() {
    boost::compute::wait_list wait;
    for (int i = 0; i != chain_length; ++i) {
      auto event = queue.enqueue_task(kernel, wait);
      wait = boost::compute::wait_list(event);
    }
    wait.wait();
    return chain_length;
  }

private:
  int chain_length;
  boost::compute::kernel kernel;
  boost::compute::command_queue queue;
};
} // anonymous namespace

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.process_cmdline(argc, argv);
  std::cerr << "Configuration for test\n" << cfg << std::endl;

  boost::compute::device device = jb::opencl::device_selector(cfg.opencl());
  boost::compute::context context(device);
  boost::compute::command_queue queue(context, device);
  typedef jb::testing::microbenchmark<fixture> benchmark;
  benchmark bm(cfg.microbenchmark());

  auto r = bm.run(context, queue);
  bm.typical_output(r);

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
