/**
 * This benchmark evaluates the performance of time_delay_estimator_many.
 *
 * It validates the assumption that FFTW performs better with an array of
 * timeseries vs. being called multiple times with a one timeseries
 * container each time.
 */
#include <jb/detail/array_traits.hpp>
#include <jb/fftw/time_delay_estimator_many.hpp>
#include <jb/testing/create_triangle_timeseries.hpp>
#include <jb/testing/delay_timeseries.hpp>
#include <jb/testing/microbenchmark.hpp>
#include <jb/testing/microbenchmark_group_main.hpp>
#include <jb/testing/sum_square.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

/// Helper types and functions to benchmark jb::fftw::time_delay_estimator.
namespace {
/**
 * Configuration parameters for bm_time_delay_estimator_many.
 *
 * Add timeseries and log parameters to the base
 * jb::testing::microbenchmark_config.
 */
class config : public jb::config_object {
public:
  config();
  config_object_constructors(config);

  void validate() const override;

  jb::config_attribute<config, jb::log::config> log;
  jb::config_attribute<config, jb::testing::microbenchmark_config>
      microbenchmark;
  jb::config_attribute<config, int> n_timeseries;
};

/// Create all the test cases.
jb::testing::microbenchmark_group<config> create_testcases();
} // anonymous namespace

int main(int argc, char* argv[]) {
  // Simply call the generic microbenchmark for a group of testcases ...
  return jb::testing::microbenchmark_group_main<config>(
      argc, argv, create_testcases());
}

namespace {
// These magic numbers are motivated by observed delays between two market
// feeds.  They assume that delay is normally around 1,250 microseconds, but
// can be as large as 6,000 microseconds.  To reliably detect the 6,000 usecs
// delays we need samples that cover at least 18,000 microseconds, assuming
// a sampling rate of 10 microseconds that is 1,800 samples, FFTs work well
// with sample sizes that are powers of 2, so we use 4096 samples.
std::chrono::microseconds const expected_delay(1250);
std::chrono::microseconds const sampling_period(10);
int const nsamples = 4096;

/**
 * Fixture for container like implementation (vector).
 *
 * Implements a call to bm_time_delay_estimator with n_timeseries iterations.
 * Since
 * estimate_delay might modify the values of the time series passed as
 * arguments, a vector
 * of n_timeseries time series is constructed with redundant data to pass one on
 * each call.
 *
 * @tparam timeseries_type container (e.g. std::vector<>) containing one time
 * series
 */
template <typename timeseries_type>
class fixture {
public:
  explicit fixture(int n_timeseries)
      : fixture(nsamples, n_timeseries) {
  }
  fixture(int size, int n_timeseries)
      : n_ts_(n_timeseries)
      , va_(n_ts_, timeseries_type(size))
      , vb_(n_ts_, timeseries_type(size))
      , estimator_(va_[0], vb_[0])
      , confidence_(va_[0])
      , tde_(va_[0])
      , sum2_(va_[0]) {
    timeseries_type a(size), b(size);
    // creates one time series...
    jb::testing::create_triangle_timeseries(size, a);
    b = jb::testing::delay_timeseries_periodic(
        a, expected_delay, sampling_period);
    sum2_ = jb::testing::sum_square(a);
    for (int i = 0; i != n_ts_; ++i) {
      va_[i] = a;
      vb_[i] = b;
    }
  }

  int run() {
    for (int i = 0; i != n_ts_; ++i) {
      estimator_.estimate_delay(confidence_, tde_, va_[i], vb_[i], sum2_);
    }
    return static_cast<int>(n_ts_ * va_.size());
  }

  using vector_timeseries_type = typename std::vector<timeseries_type>;
  using tested_type = jb::fftw::time_delay_estimator_many<timeseries_type>;
  using confidence_type = typename tested_type::confidence_type;
  using estimated_delay_type = typename tested_type::estimated_delay_type;
  using sum2_type = typename tested_type::sum2_type;

private:
  // Time series number. Number of times the estimate_delay has to be called
  int n_ts_;
  // contains n_ts_ copies of triangle shaped time series
  vector_timeseries_type va_;
  // contains n_ts_ copies of the same triangle shaped time series shifted by
  // expected_delay
  vector_timeseries_type vb_;
  // the tested type, time delay estimator (TDE)
  tested_type estimator_;
  // ignored. Argumnet to call the TDE
  confidence_type confidence_;
  // ignored. Argument to call the TDE
  estimated_delay_type tde_;
  // the sum of the square of the triangle shaped time series (anyone is valid)
  sum2_type sum2_;
};

/**
 * Fixture for boost multi array, only 2 dimensions allowed.
 *
 * @tparam T value type stores on the multi array
 * @tparam A Allocator type to allocate value types
 */
template <typename T, typename A>
class fixture<boost::multi_array<T, 2, A>> {
public:
  fixture(int n_timeseries)
      : fixture(nsamples, n_timeseries) {
  }
  fixture(int size, int n_timeseries)
      : a_(boost::extents[n_timeseries][size])
      , b_(boost::extents[n_timeseries][size])
      , estimator_(a_, b_)
      , confidence_(a_)
      , tde_(a_)
      , sum2_(a_) {
    // creates a family of n_timeseries time series of size samples each
    jb::testing::create_triangle_timeseries(size, a_);
    b_ = jb::testing::delay_timeseries_periodic(
        a_, expected_delay, sampling_period);
    sum2_ = jb::testing::sum_square(a_);
  }

  int run() {
    estimator_.estimate_delay(confidence_, tde_, a_, b_, sum2_);
    return static_cast<int>(a_.shape()[0] * a_.shape()[1]);
  }

  using timeseries_type = boost::multi_array<T, 2, A>;
  using tested_type = jb::fftw::time_delay_estimator_many<timeseries_type>;
  using confidence_type = typename tested_type::confidence_type;
  using estimated_delay_type = typename tested_type::estimated_delay_type;
  using sum2_type = typename tested_type::sum2_type;

private:
  timeseries_type a_;
  timeseries_type b_;
  tested_type estimator_;
  confidence_type confidence_;
  estimated_delay_type tde_;
  sum2_type sum2_;
};

/**
 * Create a test case for the given timeseries type.
 *
 * Returns a (type erased) functor to run the microbenchmark for the
 * give timeseries type.
 *
 * @tparam vector_type typically an instantiation of
 * boost::multi_array<> or something similar.
 */
template <typename vector_type>
std::function<void(config const&)> test_case() {
  return [](config const& cfg) {
    using benchmark = jb::testing::microbenchmark<fixture<vector_type>>;
    benchmark bm(cfg.microbenchmark());
    auto r = bm.run(cfg.n_timeseries());
    bm.typical_output(r);
  };
}

// Return the group of testcases
jb::testing::microbenchmark_group<config> create_testcases() {
  return jb::testing::microbenchmark_group<config>{
      {"float:aligned:many",
       test_case<jb::fftw::aligned_multi_array<float, 2>>()},
      {"float:aligned:single", test_case<jb::fftw::aligned_vector<float>>()},
      {"double:aligned:many",
       test_case<jb::fftw::aligned_multi_array<double, 2>>()},
      {"double:aligned:single", test_case<jb::fftw::aligned_vector<double>>()},
      {"float:unaligned:many", test_case<boost::multi_array<float, 2>>()},
      {"float:unaligned:single", test_case<std::vector<float>>()},
      {"double:unaligned:many", test_case<boost::multi_array<double, 2>>()},
      {"double:unaligned:single", test_case<std::vector<double>>()},
      {"complex:float:aligned:many",
       test_case<jb::fftw::aligned_multi_array<std::complex<float>, 2>>()},
      {"complex:float:aligned:single",
       test_case<jb::fftw::aligned_vector<std::complex<float>>>()},
      {"complex:double:aligned:many",
       test_case<jb::fftw::aligned_multi_array<std::complex<double>, 2>>()},
      {"complex:double:aligned:single",
       test_case<jb::fftw::aligned_vector<std::complex<double>>>()},
      {"complex:float:unaligned:many",
       test_case<boost::multi_array<std::complex<float>, 2>>()},
      {"complex:float:unaligned:single",
       test_case<std::vector<std::complex<float>>>()},
      {"complex:double:unaligned:many",
       test_case<boost::multi_array<std::complex<double>, 2>>()},
      {"complex:double:unaligned:single",
       test_case<std::vector<std::complex<double>>>()},
  };
}

namespace defaults {

#ifndef JB_FFTW_DEFAULT_bm_time_delay_estimator_many_test_case
#define JB_FFTW_DEFAULT_bm_time_delay_estimator_many_test_case                 \
  "float:aligned:many"
#endif // JB_FFTW_DEFAULT_bm_time_delay_estimator_many_test_case

#ifndef JB_FFTW_DEFAULT_bm_time_delay_estimator_many_n_timeseries
#define JB_FFTW_DEFAULT_bm_time_delay_estimator_many_n_timeseries 1
#endif // JB_FFTW_DEFAULT_bm_time_delay_estimator_many_n_timeseries

std::string const test_case =
    JB_FFTW_DEFAULT_bm_time_delay_estimator_many_test_case;
int constexpr n_timeseries =
    JB_FFTW_DEFAULT_bm_time_delay_estimator_many_n_timeseries;

} // namespace defaults

config::config()
    : log(desc("log", "logging"), this)
    , microbenchmark(
          desc("microbenchmark", "microbenchmark"), this,
          jb::testing::microbenchmark_config().test_case(defaults::test_case))
    , n_timeseries(
          desc("ntimeseries")
              .help(
                  "Number of timeseries as argument to compute TDE. "
                  "If microbenchmark.test_case is *:single, the fixture "
                  "executes this many calls "
                  " to compute TDE passing a container with one time series as "
                  "argument every time. "
                  "If it is *:many, the fixture uses a 2-dimension array "
                  "containing this "
                  "many time series as argument to a one time compute TDE."),
          this, defaults::n_timeseries) {
}

void config::validate() const {
  log().validate();
  microbenchmark().validate();
  if (n_timeseries() < 1) {
    std::ostringstream os;
    os << "n_timeseries must be > 0 (" << n_timeseries() << ")";
    throw jb::usage(os.str(), 1);
  }
}

} // anonymous namespace
