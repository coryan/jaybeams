#include <jb/opencl/config.hpp>
#include <jb/opencl/device_selector.hpp>
#include <jb/testing/microbenchmark.hpp>
#include <jb/testing/microbenchmark_group_main.hpp>
#include <jb/complex_traits.hpp>
#include <jb/log.hpp>

#include <boost/compute/algorithm/max_element.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/context.hpp>
#include <boost/compute/types/complex.hpp>
#include <iostream>
#include <stdexcept>
#include <string>

/// Functions and types to benchmark the argmax reduction based on
/// Boost.Compute
namespace {
class config : public jb::config_object {
public:
  config();
  config_object_constructors(config);

  jb::config_attribute<config, jb::testing::microbenchmark_config>
      microbenchmark;
  jb::config_attribute<config, jb::log::config> log;
  jb::config_attribute<config, jb::opencl::config> opencl;
};

/// Return a table with all the testcases ..
jb::testing::microbenchmark_group<config> create_testcases();
} // anonymous namespace

int main(int argc, char* argv[]) {
  auto testcases = create_testcases();
  return jb::testing::microbenchmark_group_main<config>(argc, argv, testcases);
}

namespace {
config::config()
    : microbenchmark(
          desc("microbenchmark", "microbenchmark"), this,
          jb::testing::microbenchmark_config().test_case("gpu:float"))
    , log(desc("log", "log"), this)
    , opencl(desc("opencl", "opencl"), this) {
}

constexpr int default_size() {
  return 32768;
}

template <typename T>
std::size_t gpu_argmax(
    boost::compute::vector<T> const& dev,
    boost::compute::command_queue& queue) {
  typedef T value_type;
  BOOST_COMPUTE_FUNCTION(
      bool, less_real, (value_type const& a, value_type const& b),
      { return a < b; });

  return std::distance(
      dev.begin(),
      boost::compute::max_element(dev.begin(), dev.end(), less_real, queue));
}

template <typename T>
std::size_t gpu_argmax(
    boost::compute::vector<std::complex<T>> const& dev,
    boost::compute::command_queue& queue) {
  typedef std::complex<T> value_type;
  BOOST_COMPUTE_FUNCTION(
      bool, less_real, (value_type const& a, value_type const& b),
      { return a.x < b.x; });

  return std::distance(
      dev.begin(),
      boost::compute::max_element(dev.begin(), dev.end(), less_real, queue));
}

template <typename T>
std::size_t cpu_argmax(std::vector<T> const& host) {
  typedef T value_type;
  auto less_real = [](value_type const& a, value_type const& b) {
    return std::real(a) < std::real(b);
  };

  return std::distance(
      host.begin(), std::max_element(host.begin(), host.end(), less_real));
}

/**
 * The benchmark fixture.
 *
 * @tparam value_type the values stored in the vector.
 * @tparam use_gpu if true, the computation is executed using
 * Boost.Compute, and presumably OpenCL in the GPU.
 */
template <typename value_type, bool use_gpu>
class fixture {
public:
  /// Constructor
  fixture(boost::compute::context& context, boost::compute::command_queue& q)
      : fixture(default_size(), context, q) {
  }

  /// Constructor with a size
  fixture(
      int size, boost::compute::context& context,
      boost::compute::command_queue& q)
      : dev(size, context)
      , host(size)
      , queue(q)
      , unused(0) {
    int counter = 0;
    for (auto& v : host) {
      v = value_type(++counter);
    }
    boost::compute::copy(host.begin(), host.end(), dev.begin(), queue);
    queue.finish();
  }

  int run() {
    if (use_gpu) {
      unused += gpu_argmax(dev, queue);
    } else {
      unused += cpu_argmax(host);
    }
    return static_cast<int>(host.size());
  }

  /// Disable aggressive optimizations
  std::size_t dummy() const {
    return unused;
  }

private:
  boost::compute::vector<value_type> dev;
  std::vector<value_type> host;
  boost::compute::command_queue queue;
  std::size_t unused;
};

/**
 * Create one of the test-cases for the microbenchmark.
 */
template <typename value_type, bool use_gpu>
std::function<void(config const&)> benchmark_test_case() {
  return [](config const& cfg) {
    boost::compute::device device = jb::opencl::device_selector(cfg.opencl());
    boost::compute::context context(device);
    boost::compute::command_queue queue(context, device);

    using benchmark = jb::testing::microbenchmark<fixture<value_type, use_gpu>>;
    benchmark bm(cfg.microbenchmark());

    auto r = bm.run(context, queue);
    bm.typical_output(r);
  };
}

/// A table with all the microbenchmark cases
jb::testing::microbenchmark_group<config> create_testcases() {
  return jb::testing::microbenchmark_group<config>{
      {"gpu:complex:float", benchmark_test_case<std::complex<float>, true>()},
      {"gpu:complex:double", benchmark_test_case<std::complex<double>, true>()},
      {"cpu:complex:float", benchmark_test_case<std::complex<float>, false>()},
      {"cpu:complex:double",
       benchmark_test_case<std::complex<double>, false>()},
      {"gpu:float", benchmark_test_case<float, true>()},
      {"gpu:double", benchmark_test_case<double, true>()},
      {"cpu:float", benchmark_test_case<float, false>()},
      {"cpu:double", benchmark_test_case<double, false>()},
  };
}
} // anonymous namespace
