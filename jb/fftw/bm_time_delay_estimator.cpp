/**
 * A microbenchmark for different instantiations of jb::time_delay_estimator.
 */
#include <jb/fftw/time_delay_estimator.hpp>
#include <jb/testing/create_square_timeseries.hpp>
#include <jb/testing/delay_timeseries.hpp>
#include <jb/testing/microbenchmark.hpp>
#include <jb/testing/microbenchmark_group_main.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

// These magic numbers are motivated by observed delays between two market
// feeds.  They assume that delay is normally around 1,250 microseconds, but
// can be as large as 6,000 microseconds.  To reliably detect the 6,000 usecs
// delays we need samples that cover at least 18,000 microseconds, assuming
// a sampling rate of 10 microseconds that is 1,800 samples, FFTs work well
// with sample sizes that are powers of 2, so we use 2048 samples.
std::chrono::microseconds const expected_delay(1250);
std::chrono::microseconds const sampling_period(10);
int const nsamples = 2048;

/// Configuration parameters for bm_time_delay_estimator
class config : public jb::config_object {
public:
  config();
  config_object_constructors(config);

  void validate() const override;

  jb::config_attribute<config, jb::log::config> log;
  jb::config_attribute<config, jb::testing::microbenchmark_config>
      microbenchmark;
};

/**
 * The fixture for this microbenchmark.
 *
 * Run the microbenchmark with an specific timeseries type (vector
 * vs. aligned_vector, float vs. complex vs. double).
 *
 * @tparam timeseries_type the type of timeseries, typically an
 * instance of std::vector<> or jb::fftw::aligned_vector<>.
 */
template <typename timeseries_type>
class fixture {
public:
  /// Define the tested estimator type.
  using tested = jb::fftw::time_delay_estimator<timeseries_type>;

  /// Constructor with default size
  fixture()
      : fixture(nsamples) {
  }

  /// Constructor with given size
  fixture(int size)
      : a(size)
      , b(size)
      , estimator(a, b) {
    // Initialize the values in the timeseries.  The estimator is
    // already initialized and ready to run ...
    jb::testing::create_square_timeseries(size, a);
    b = jb::testing::delay_timeseries_periodic(
        a, expected_delay, sampling_period);
  }

  /// Run a single iteration of the estimation
  void run() {
    // ... perform the estimation ...
    auto e = estimator.estimate_delay(a, b);
    // ... we could ignore the value, but just in case ...
    if (not e.first) {
      throw std::runtime_error("estimation failed");
    }
  }

private:
  timeseries_type a;
  timeseries_type b;
  tested estimator;
};

/**
 * Create a test case for the given timeseries type.
 *
 * Returns a (type erased) functor to run the microbenchmark for the
 * give timeseries type.
 *
 * @tparam timeseries_type typically an instantiation of std::vector<>
 * or jb::fftw::aligned_vector<>.
 */
template <typename timeseries_type>
std::function<void(config const&)> test_case() {
  return [](config const& cfg) {
    // Create a microbenchmark with the right fixture, initialize from
    // the configuration parameter ...
    using benchmark = jb::testing::microbenchmark<fixture<timeseries_type>>;
    benchmark bm(cfg.microbenchmark());

    // ... run the microbenchmark ...
    auto r = bm.run();
    // ... output in the most commonly used format for JayBeams
    // microbenchmarks ...
    bm.typical_output(r);
  };
}

/// A table with all the microbenchmark cases.
jb::testing::microbenchmark_group<config> testcases{
    {"float:aligned", test_case<jb::fftw::aligned_vector<float>>()},
    {"double:aligned", test_case<jb::fftw::aligned_vector<double>>()},
    {"float:unaligned", test_case<std::vector<float>>()},
    {"double:unaligned", test_case<std::vector<double>>()},
    {"complex:float:aligned",
     test_case<jb::fftw::aligned_vector<std::complex<float>>>()},
    {"complex:double:aligned",
     test_case<jb::fftw::aligned_vector<std::complex<double>>>()},
    {"complex:float:unaligned", test_case<std::vector<std::complex<float>>>()},
    {"complex:double:unaligned",
     test_case<std::vector<std::complex<double>>>()},
};
} // anonymous namespace

int main(int argc, char* argv[]) {
  // Simply call the generic microbenchmark for a group of testcases ...
  return jb::testing::microbenchmark_group_main<config>(argc, argv, testcases);
}

namespace {
namespace defaults {
#ifndef JB_FFTW_DEFAULT_fftw_bm_time_delay_estimator_test_case
#define JB_FFTW_DEFAULT_fftw_bm_time_delay_estimator_test_case "float:aligned"
#endif // JB_FFTW_DEFAULT_fftw_bm_time_delay_estimator_test_case

std::string const test_case =
    JB_FFTW_DEFAULT_fftw_bm_time_delay_estimator_test_case;
} // namespace defaults

config::config()
    : log(desc("log", "logging"), this)
    , microbenchmark(
          desc("microbenchmark", "microbenchmark"), this,
          jb::testing::microbenchmark_config().test_case(defaults::test_case)) {
}

void config::validate() const {
  log().validate();
  microbenchmark().validate();
}

} // anonymous namespace
