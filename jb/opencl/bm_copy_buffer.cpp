#include <jb/opencl/config.hpp>
#include <jb/opencl/device_selector.hpp>
#include <jb/opencl/copy_to_host_async.hpp>
#include <jb/testing/microbenchmark.hpp>

#include <boost/compute/algorithm/copy.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/container/vector.hpp>
#include <cstdint>
#include <iostream>
#include <string>
#include <stdexcept>
#include <unistd.h>

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

// By default use a single page, at least a typical page size
int get_page_size() {
  return static_cast<int>(sysconf(_SC_PAGESIZE));
}

int default_size() {
  int pagesize = get_page_size();
  return pagesize;
}


template<bool upload>
class fixture {
 public:
  fixture(
      boost::compute::context& context,
      boost::compute::command_queue& q,
      bool aligned)
      : fixture(default_size(), context, q, aligned)
  {}
  fixture(
      int size,
      boost::compute::context& context,
      boost::compute::command_queue& q,
      bool aligned)
      : dev(size / sizeof(int), context)
      , host((size + get_page_size()) / sizeof(int))
      , queue(q)
      , start(host.begin())
      , end(host.begin() + size / sizeof(int))
  {
    int pagesize = get_page_size();
    std::intptr_t ptr = reinterpret_cast<std::intptr_t>(
        boost::addressof(*start));
    if (not aligned) {
      if (ptr % pagesize == 0) {
        // simply increment the ptr to get an unaligned buffer 
        ++start;
        ++end;
      }
      return;
    }
    while (ptr % pagesize != 0 and ++start != host.end()
           and ++end != host.end()) {
      ptr = reinterpret_cast<std::intptr_t>(boost::addressof(*start));
    }
    if (ptr % pagesize != 0) {
      throw std::runtime_error("Could not align buffer");
    }
  }

  void run() {
    boost::compute::copy(start, end, dev.begin(), queue);
  }

 private:
  boost::compute::vector<int> dev;
  std::vector<int> host;
  boost::compute::command_queue queue;
  std::vector<int>::iterator start;
  std::vector<int>::iterator end;
};

/// Implement the download case
template<>
void fixture<false>::run() {
  boost::compute::copy(dev.begin(), dev.end(), start, queue);
}

template<bool upload>
void benchmark_test_case(config const& cfg, bool aligned) {
  boost::compute::device device = jb::opencl::device_selector(cfg.opencl());
  boost::compute::context context(device);
  boost::compute::command_queue queue(context, device);
  typedef jb::testing::microbenchmark<fixture<upload>> benchmark;
  benchmark bm(cfg.benchmark());

  auto r = bm.run(context, queue, aligned);
  bm.typical_output(r);
}

} // anonymous namespace

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.benchmark(
      jb::testing::microbenchmark_config().test_case("upload:aligned"))
      .process_cmdline(argc, argv);

  std::cerr << "Configuration for test\n" << cfg << std::endl;

  auto test_case = cfg.benchmark().test_case();
  if (test_case == "upload:aligned") {
    benchmark_test_case<true>(cfg, true);
  } else if (test_case == "upload:misaligned") {
    benchmark_test_case<true>(cfg, false);
  } else if (test_case == "download:aligned") {
    benchmark_test_case<false>(cfg, true);
  } else if (test_case == "download:misaligned") {
    benchmark_test_case<false>(cfg, false);
  } else {
    std::ostringstream os;
    os << "Unknown test case (" << test_case << ")" << std::endl;
    os << " --test-case must be one of"
       << ": upload:aligned"
       << ", upload:misaligned"
       << ", download:saligned"
       << ", download:misaligned"
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

