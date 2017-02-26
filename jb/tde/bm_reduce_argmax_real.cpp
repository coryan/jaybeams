#include <jb/opencl/config.hpp>
#include <jb/opencl/device_selector.hpp>
#include <jb/testing/create_random_timeseries.hpp>
#include <jb/testing/microbenchmark.hpp>
#include <jb/complex_traits.hpp>

#include <boost/compute/algorithm/max_element.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/context.hpp>
#include <boost/compute/types/complex.hpp>
#include <iostream>
#include <random>
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

template <typename value_type, bool use_gpu = false>
class fixture {
public:
  fixture(boost::compute::context& context, boost::compute::command_queue& q)
      : fixture(default_size(), context, q) {
  }
  fixture(
      int size, boost::compute::context& context,
      boost::compute::command_queue& q)
      : dev(size, context)
      , host(size)
      , queue(q)
      , unused(0) {
    typedef typename jb::extract_value_type<value_type>::precision precision_t;
    unsigned int seed = std::random_device()();
    std::mt19937 gen(seed);
    std::uniform_real_distribution<precision_t> dis(-1000, 1000);
    auto generator = [&gen, &dis]() { return dis(gen); };
    std::cerr << "SEED = " << seed << std::endl;
    jb::testing::create_random_timeseries(generator, size, host);
    boost::compute::copy(host.begin(), host.end(), dev.begin(), queue);
  }

  int run() {
    if (use_gpu) {
      unused += gpu_argmax(dev, queue);
    } else {
      unused += cpu_argmax(host);
    }
    return static_cast<int>(host.size());
  }

  std::size_t dummy() const {
    return unused;
  }

private:
  boost::compute::vector<value_type> dev;
  std::vector<value_type> host;
  boost::compute::command_queue queue;
  std::size_t unused;
};

template <typename value_type, bool use_gpu>
void benchmark_test_case(config const& cfg) {
  boost::compute::device device = jb::opencl::device_selector(cfg.opencl());
  boost::compute::context context(device);
  boost::compute::command_queue queue(context, device);
  typedef jb::testing::microbenchmark<fixture<value_type, use_gpu>> benchmark;
  benchmark bm(cfg.benchmark());

  auto r = bm.run(context, queue);
  bm.typical_output(r);
}

} // anonymous namespace

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.benchmark(
         jb::testing::microbenchmark_config().test_case("gpu:complex:float"))
      .process_cmdline(argc, argv);

  std::cerr << "Configuration for test\n" << cfg << std::endl;

  auto test_case = cfg.benchmark().test_case();
  if (test_case == "gpu:complex:float") {
    benchmark_test_case<std::complex<float>, true>(cfg);
  } else if (test_case == "gpu:complex:double") {
    benchmark_test_case<std::complex<double>, true>(cfg);
  } else if (test_case == "cpu:complex:float") {
    benchmark_test_case<std::complex<float>, false>(cfg);
  } else if (test_case == "cpu:complex:double") {
    benchmark_test_case<std::complex<double>, false>(cfg);
  } else if (test_case == "gpu:float") {
    benchmark_test_case<float, true>(cfg);
  } else if (test_case == "gpu:double") {
    benchmark_test_case<double, true>(cfg);
  } else if (test_case == "cpu:float") {
    benchmark_test_case<float, false>(cfg);
  } else if (test_case == "cpu:double") {
    benchmark_test_case<double, false>(cfg);
  } else {
    std::ostringstream os;
    os << "Unknown test case (" << test_case << ")" << std::endl;
    os << " --test-case must be one of"
       << ": gpu:complex:float"
       << ", gpu:complex:double"
       << ", cpu:complex:float"
       << ", cpu:complex:double"
       << ", gpu:float"
       << ", gpu:double"
       << ", cpu:float"
       << ", cpu:double" << std::endl;
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
