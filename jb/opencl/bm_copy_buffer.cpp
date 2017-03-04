#include <jb/opencl/config.hpp>
#include <jb/opencl/copy_to_host_async.hpp>
#include <jb/opencl/device_selector.hpp>
#include <jb/testing/microbenchmark.hpp>
#include <jb/testing/microbenchmark_group_main.hpp>

#include <boost/compute/algorithm/copy.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/container/vector.hpp>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>

namespace {
class config : public jb::config_object {
public:
  config()
      : microbenchmark(
            desc("microbenchmark"), this,
            jb::testing::microbenchmark_config().test_case("upload:aligned"))
      , log(desc("log", "log"), this)
      , opencl(desc("opencl"), this) {
  }

  jb::config_attribute<config, jb::testing::microbenchmark_config>
      microbenchmark;
  jb::config_attribute<config, jb::log::config> log;
  jb::config_attribute<config, jb::opencl::config> opencl;
};

/// Create all the testcases
jb::testing::microbenchmark_group<config> create_test_cases();

} // anonymous namespace

int main(int argc, char* argv[]) {
  // Simply call the generic microbenchmark for a group of testcases
  // ...
  auto testcases = create_test_cases();
  return jb::testing::microbenchmark_group_main<config>(argc, argv, testcases);
}

namespace {
// By default use a single page, at least a typical page size
int get_page_size() {
  return static_cast<int>(sysconf(_SC_PAGESIZE));
}

int default_size() {
  int pagesize = get_page_size();
  return pagesize;
}

template <bool upload>
class fixture {
public:
  fixture(
      boost::compute::context& context, boost::compute::command_queue& q,
      bool aligned)
      : fixture(default_size(), context, q, aligned) {
  }
  fixture(
      int size, boost::compute::context& context,
      boost::compute::command_queue& q, bool aligned)
      : dev(size / sizeof(int), context)
      , host((size + get_page_size()) / sizeof(int))
      , queue(q)
      , start(host.begin())
      , end(host.begin() + size / sizeof(int)) {
    int pagesize = get_page_size();
    std::intptr_t ptr =
        reinterpret_cast<std::intptr_t>(boost::addressof(*start));
    if (not aligned) {
      if (ptr % pagesize == 0) {
        // simply increment the ptr to get an unaligned buffer
        ++start;
        ++end;
      }
      return;
    }
    while (ptr % pagesize != 0 and ++start != host.end() and
           ++end != host.end()) {
      ptr = reinterpret_cast<std::intptr_t>(boost::addressof(*start));
    }
    if (ptr % pagesize != 0) {
      throw std::runtime_error("Could not align buffer");
    }
  }

  int run() {
    boost::compute::copy(start, end, dev.begin(), queue);
    return static_cast<int>(dev.size());
  }

private:
  boost::compute::vector<int> dev;
  std::vector<int> host;
  boost::compute::command_queue queue;
  std::vector<int>::iterator start;
  std::vector<int>::iterator end;
};

/// Implement the download case
template <>
int fixture<false>::run() {
  boost::compute::copy(dev.begin(), dev.end(), start, queue);
  return static_cast<int>(dev.size());
}

template <bool upload>
void benchmark_test_case(config const& cfg, bool aligned) {
}

template <bool upload, bool aligned>
std::function<void(config const&)> test_case() {
  return [](config const& cfg) {
    boost::compute::device device = jb::opencl::device_selector(cfg.opencl());
    boost::compute::context context(device);
    boost::compute::command_queue queue(context, device);
    typedef jb::testing::microbenchmark<fixture<upload>> benchmark;
    benchmark bm(cfg.microbenchmark());

    auto r = bm.run(context, queue, aligned);
    bm.typical_output(r);
  };
}

jb::testing::microbenchmark_group<config> create_test_cases() {
  return jb::testing::microbenchmark_group<config>{
      {"upload:aligned", test_case<true, true>()},
      {"upload:misaligned", test_case<true, false>()},
      {"download:aligned", test_case<false, true>()},
      {"download:misaligned", test_case<false, false>()},
  };
}

} // anonymous namespace
