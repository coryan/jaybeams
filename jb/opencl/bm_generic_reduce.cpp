#include <jb/opencl/device_selector.hpp>
#include <jb/opencl/generic_reduce.hpp>
#include <jb/opencl/microbenchmark_config.hpp>
#include <jb/testing/initialize_mersenne_twister.hpp>
#include <jb/testing/microbenchmark.hpp>
#include <jb/testing/microbenchmark_group_main.hpp>
#include <jb/complex_traits.hpp>

#include <boost/compute/algorithm/reduce.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/container/vector.hpp>

#include <vector>

/// Functions and types to benchmark the generic reduction functions
namespace {
#ifndef JB_OPENCL_bm_generic_reduce_minimum_size
#define JB_OPENCL_bm_generic_reduce_minimum_size 16
#endif // JB_OPENCL_bm_generic_reduce_minimum_size

/**
 * The configuration class for this benchmark.
 */
class config : public jb::opencl::microbenchmark_config {
public:
  config();
  config_object_constructors(config);

  jb::config_attribute<config, bool> randomize_size;
  jb::config_attribute<config, bool> copy_data;
};

/// Return a table with all the testcases ..
jb::testing::microbenchmark_group<config> create_testcases();
} // anonymous namespace

int main(int argc, char* argv[]) {
  auto testcases = create_testcases();
  return jb::testing::microbenchmark_group_main<config>(argc, argv, testcases);
}

namespace {
std::string randomize_size_help() {
  std::ostringstream os;
  os << "If true, the size is randomized in each iteration."
     << "  This is useful when trying to build regression models,"
     << " but not when trying to fine tune algorithms."
     << "  The random distributes uniformly "
     << "between " << JB_OPENCL_bm_generic_reduce_minimum_size
     << " and the configured size of the test.";
  return os.str();
}

config::config()
    : microbenchmark_config()
    , randomize_size(
          desc("randomize-size").help(randomize_size_help()), this, true)
    , copy_data(
          desc("copy-data")
              .help(
                  "If set, the test copies fresh data to the OpenCL device"
                  " on each iteration.  Effectively that tests copy + "
                  "reduction."
                  " Disabling this flag tests reduction assuming the data is"
                  " already on the device."),
          this, true) {
}

template <typename T>
struct opencl_type_traits {};

template <>
struct opencl_type_traits<double> {
  static char const* macro_prefix() {
    return "DBL_";
  }
};

template <>
struct opencl_type_traits<float> {
  static char const* macro_prefix() {
    return "FLT_";
  }
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
    return std::string("*") + lhs + " = " +
           opencl_type_traits<T>::macro_prefix() + "MAX;";
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

/**
 * Base fixture to benchmark OpenCL reductions.
 */
template <typename T>
class base_fixture {
public:
  /// Constructor for default size
  base_fixture(
      config const& cfg, boost::compute::context& context,
      boost::compute::command_queue& q)
      : base_fixture(cfg, context, q) {
  }

  /// Constructor with known size
  base_fixture(
      int size, config const& cfg, boost::compute::context& context,
      boost::compute::command_queue& q)
      : host_(size)
      , device_(size, context)
      , queue_(q)
      , generator_(
            jb::testing::initialize_mersenne_twister<std::mt19937_64>(
                0, jb::testing::default_initialization_marker))
      , avoid_optimization_(0)
      , cfg_(cfg) {
    int counter = 0;
    for (auto& i : host_) {
      i = size + 1 - ++counter;
    }
    boost::compute::copy(host_.begin(), host_.end(), device_.begin(), queue_);
    queue_.finish();
  }

  /// Run each iteration with a random size, part of testing the
  void iteration_setup() {
    if (cfg_.randomize_size()) {
      // ... pick a random size to execute the test at ...
      iteration_size_ = std::uniform_int_distribution<>(
          JB_OPENCL_bm_generic_reduce_minimum_size,
          host_.size() - 1)(generator_);
    }
  }

  /// Return the value accumulated through all iterations
  T avoid_optimization() const {
    return avoid_optimization_;
  }

protected:
  std::vector<T> host_;
  boost::compute::vector<T> device_;
  boost::compute::command_queue queue_;
  std::mt19937_64 generator_;
  int iteration_size_;
  T avoid_optimization_;
  config cfg_;
};

/**
 * A fixture to benchmark the boost::compute::reduce function.
 */
template <typename T>
class boost_fixture : public base_fixture<T> {
public:
  /// Constructor for default size
  boost_fixture(
      config const& cfg, boost::compute::context& context,
      boost::compute::command_queue& q)
      : boost_fixture(1024, cfg, context, q) {
  }

  /// Constructor with known size
  boost_fixture(
      int size, config const& cfg, boost::compute::context& context,
      boost::compute::command_queue& q)
      : base_fixture<T>(size, cfg, context, q) {
  }

  int run() {
    if (this->cfg_.copy_data()) {
      // ... copy the first iteration_size_ elements to the device ...
      (void)boost::compute::copy(
          this->host_.begin(), this->host_.begin() + this->iteration_size_,
          this->device_.begin(), this->queue_);
    }
    // ... run a reduction on the device to pick the minimum value
    // across those elements ...
    T result = 0;
    boost::compute::reduce(
        this->device_.begin(), this->device_.begin() + this->iteration_size_,
        &result, boost::compute::min<T>(), this->queue_);
    this->queue_.finish();
    this->avoid_optimization_ += result;
    // ... return the iteration size ...
    return this->iteration_size_;
  }
};

/**
 * A fixture to benchmark the boost::compute::reduce function.
 */
template <typename T>
class boost_async_fixture : public base_fixture<T> {
public:
  /// Constructor for default size
  boost_async_fixture(
      config const& cfg, boost::compute::context& context,
      boost::compute::command_queue& q)
      : boost_async_fixture(1024, cfg, context, q) {
  }

  /// Constructor with known size
  boost_async_fixture(
      int size, config const& cfg, boost::compute::context& context,
      boost::compute::command_queue& q)
      : base_fixture<T>(size, cfg, context, q) {
  }

  int run() {
    // ... copy the first iteration_size_ elements to the device ...
    if (this->cfg_.copy_data()) {
      auto end = boost::compute::copy_async(
          this->host_.begin(), this->host_.begin() + this->iteration_size_,
          this->device_.begin(), this->queue_);
      // ... enqueue a barrier to only start the reduction once the copy
      // has completed ...
      this->queue_.enqueue_barrier();
    }
    // ... run a reduction on the device to pick the minimum value
    // across those elements ...
    T result = 0;
    boost::compute::reduce(
        this->device_.begin(), this->device_.begin() + this->iteration_size_,
        &result, boost::compute::min<T>(), this->queue_);
    this->queue_.finish();
    this->avoid_optimization_ += result;
    // ... return the iteration size ...
    return this->iteration_size_;
  }
};

/**
 * A fixture to benchmark the boost::compute::reduce function.
 */
template <typename T>
class generic_reduce_fixture : public base_fixture<T> {
public:
  /// Constructor for default size
  generic_reduce_fixture(
      config const& cfg, boost::compute::context& context,
      boost::compute::command_queue& q)
      : generic_reduce_fixture(1024, cfg, context, q) {
  }

  /// Constructor with known size
  generic_reduce_fixture(
      int size, config const& cfg, boost::compute::context& context,
      boost::compute::command_queue& q)
      : base_fixture<T>(size, cfg, context, q)
      , reducer_(size, q) {
  }

  int run() {
    boost::compute::wait_list wl;
    if (this->cfg_.copy_data()) {
      // ... copy the first iteration_size_ elements to the device ...
      auto end = boost::compute::copy_async(
          this->host_.begin(), this->host_.begin() + this->iteration_size_,
          this->device_.begin(), this->queue_);
      wl = boost::compute::wait_list(end.get_event());
    }
    // ... run a reduction on the device to pick the minimum value
    // across those elements ...
    auto result = reducer_.execute(
        this->device_.begin(), this->device_.begin() + this->iteration_size_,
        wl);
    result.wait();
    this->avoid_optimization_ += *result.get();
    // ... return the iteration size ...
    return this->iteration_size_;
  }

private:
  reduce_min<T> reducer_;
};

/**
 * A fixture to benchmark the boost::compute::reduce function.
 */
template <typename T>
class std_fixture : public base_fixture<T> {
public:
  /// Constructor for default size
  std_fixture(
      config const& cfg, boost::compute::context& context,
      boost::compute::command_queue& q)
      : std_fixture(1024, cfg, context, q) {
  }

  /// Constructor with known size
  std_fixture(
      int size, config const& cfg, boost::compute::context& context,
      boost::compute::command_queue& q)
      : base_fixture<T>(size, cfg, context, q) {
  }

  int run() {
    auto iterator = std::min_element(
        this->host_.begin(), this->host_.begin() + this->iteration_size_);
    this->avoid_optimization_ += *iterator;
    return this->iteration_size_;
  }
};

/**
 * Create a microbenchmark test-case.
 */
template <typename fixture_type>
std::function<void(config const&)> test_case() {
  return [](config const& cfg) {
    boost::compute::device device = jb::opencl::device_selector(cfg.opencl());
    boost::compute::context context(device);
    boost::compute::command_queue queue(context, device);

    std::cerr << "device=" << device.name() << std::endl;

    using benchmark = jb::testing::microbenchmark<fixture_type>;
    benchmark bm(cfg.microbenchmark());

    auto r = bm.run(cfg, context, queue);
    bm.typical_output(r);
  };
}

/// A table with all the microbenchmark cases
jb::testing::microbenchmark_group<config> create_testcases() {
  return jb::testing::microbenchmark_group<config>{
      {"boost:float", test_case<boost_fixture<float>>()},
      {"boost:double", test_case<boost_fixture<double>>()},
      {"boost_async:float", test_case<boost_async_fixture<float>>()},
      {"boost_async:double", test_case<boost_async_fixture<double>>()},
      {"generic_reduce:float", test_case<generic_reduce_fixture<float>>()},
      {"generic_reduce:double", test_case<generic_reduce_fixture<double>>()},
      {"std:float", test_case<std_fixture<float>>()},
      {"std:double", test_case<std_fixture<double>>()},
  };
}
} // anonymous namespace
