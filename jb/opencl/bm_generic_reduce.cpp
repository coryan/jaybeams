#define BOOST_COMPUTE_DEBUG_KERNEL_COMPILATION Y
#include <jb/opencl/config.hpp>
#include <jb/opencl/device_selector.hpp>
#include <jb/opencl/generic_reduce.hpp>
#include <jb/testing/microbenchmark.hpp>
#include <jb/testing/microbenchmark_group_main.hpp>
#include <jb/complex_traits.hpp>

#include <boost/compute/command_queue.hpp>
#include <boost/compute/algorithm/reduce.hpp>
#include <boost/compute/container/vector.hpp>

#include <vector>

/// Functions and types to benchmark the generic reduction functions
namespace {
/**
 * The configuration class for this benchmark.
 */
class config : public jb::config_object {
public:
  config()
      : microbenchmark(
            desc("microbenchmark", "microbenchmark"), this,
            jb::testing::microbenchmark_config().test_case("float:boost"))
      , log(desc("log", "log"), this)
      , opencl(desc("opencl"), this) {
  }

  jb::config_attribute<config, jb::testing::microbenchmark_config>
      microbenchmark;
  jb::config_attribute<config, jb::log::config> log;
  jb::config_attribute<config, jb::opencl::config> opencl;
};

template<typename T>
struct opencl_type_traits {};

template<>
struct opencl_type_traits<double> {
  static char const* macro_prefix() { return "DBL_"; }
};

template<>
struct opencl_type_traits<float> {
  static char const* macro_prefix() { return "FLT_"; }
};

/**
 * A reducer to drive the test, find the minimum value.
 */
template <typename T>
class reduce_min : public jb::opencl::generic_reduce<reduce_min<T>, T, T> {
public:
  reduce_min(std::size_t size, boost::compute::command_queue const& queue)
      : jb::opencl::generic_reduce<reduce_min<T>, T, T>(size, queue) {
  }

  /// @returns the body of the initialization function
  static std::string initialize_body(char const* lhs) {
    return std::string("*") + lhs + " = "
           << opencl_type_traits<T>::macro_prefix() << "MAX;";
  }

  /// @returns the body of the transform function
  static std::string
  transform_body(char const* lhs, char const* value, char const*) {
    return std::string("*") + lhs + " = *" + value + ";";
  }

  /// @returns the body of the combine function
  static std::string combine_body(char const* accumulated, char const* value) {
    return std::string("*") + accumulated + " = min(*" + accumulated + ", *" +
           value + ");";
  }
};

template <typename T>
class boost_fixture {
public:
  boost_fixture(
      boost::compute::context& context, boost::compute::command_queue& q)
      : boost_fixture(1024, context, q) {
  }
  boost_fixture(
      int size, boost::compute::context& context,
      boost::compute::command_queue& q)
      : host_(size)
      , device_(size, context)
      , queue_(q) {
  }

  void iteration_setup() {
    int counter = 0;
    for (auto & i : host_) {
      i = ++counter;
    }
  }

  void run() {
    boost::compute::copy(host_.begin(), host_.end(), device_.begin(), queue_);
    float result = 0;
    boost::compute::reduce(
        device_.begin(), device_.end(), &result, boost::compute::plus<T>(),
        queue_);
  }

private:
  std::vector<T> host_;
  boost::compute::vector<T> device_;
  boost::compute::command_queue queue_;
};

/**
 * Create a microbenchmark test-case
 */
template <typename T>
std::function<void(config const&)> boost_test() {
  return [](config const& cfg) {
    boost::compute::device device = jb::opencl::device_selector(cfg.opencl());
    boost::compute::context context(device);
    boost::compute::command_queue queue(context, device);

    std::cerr << "device=" << device.name() << std::endl;
    
    using benchmark = jb::testing::microbenchmark<boost_fixture<T>>;
    benchmark bm(cfg.microbenchmark());

    auto r = bm.run(context, queue);
    bm.typical_output(r);
  };
}

/// A table with all the microbenchmark cases
jb::testing::microbenchmark_group<config> testcases{
  {"boost:float", boost_test<float>()},
  {"boost:double", boost_test<double>()},
};

} // anonymous namespace

int main(int argc, char* argv[]) {
  return jb::testing::microbenchmark_group_main<config>(argc, argv, testcases);
}
