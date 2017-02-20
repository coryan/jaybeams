#include <jb/detail/array_traits.hpp>
#include <jb/fftw/time_delay_estimator_many.hpp>
#include <jb/testing/create_triangle_timeseries.hpp>
#include <jb/testing/delay_timeseries.hpp>
#include <jb/testing/microbenchmark.hpp>
#include <jb/testing/sum_square.hpp>
#include <jb/log.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * This benchmark is implemented to evaluate the performance of
 * time_delay_estimator_many.
 *
 * time_delay_estimator_many was implemented under the assumption that FFTW will
 * perform
 * better if we pass a multi array container with n time series in one call,
 * than having
 * to call the library n times passing a container (like vector) with one time
 * series each
 * time
 */

namespace {

// We expect delays to be around 1250 microseconds, max delays are
// around 6,000, so we need at least a 3*6000 ~ 18,000 microsecond
// baseline.  Rounded up to a power of 2 that is 32768.  Sampling
// every 16 microseconds that is 32768 / 16.
std::chrono::microseconds const expected_delay(1250);
std::chrono::microseconds const sampling_period(16);
int nsamples = 32768 / 16;

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
  fixture(int n_timeseries)
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

  void run() {
    for (int i = 0; i != n_ts_; ++i) {
      estimator_.estimate_delay(confidence_, tde_, va_[i], vb_[i], sum2_);
    }
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

  void run() {
    estimator_.estimate_delay(confidence_, tde_, a_, b_, sum2_);
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

template <typename vector_type>
void benchmark_test_case(config const& cfg) {
  using benchmark = jb::testing::microbenchmark<fixture<vector_type>>;
  benchmark bm(cfg.microbenchmark());
  auto r = bm.run(cfg.n_timeseries());
  bm.typical_output(r);
}

} // anonymous namespace

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.process_cmdline(argc, argv);

  // initialize the logging framework ...
  jb::log::init(cfg.log());
  if (cfg.microbenchmark().verbose()) {
    JB_LOG(info) << "Configuration for test\n" << cfg << "\n";
  }

  std::string test_case = cfg.microbenchmark().test_case();
  if (test_case == "float:aligned:many") {
    benchmark_test_case<jb::fftw::aligned_multi_array<float, 2>>(cfg);
  } else if (test_case == "float:aligned:single") {
    benchmark_test_case<jb::fftw::aligned_vector<float>>(cfg);
  } else if (test_case == "double:aligned:many") {
    benchmark_test_case<jb::fftw::aligned_multi_array<double, 2>>(cfg);
  } else if (test_case == "double:aligned:single") {
    benchmark_test_case<jb::fftw::aligned_vector<double>>(cfg);
  } else if (test_case == "float:unaligned:many") {
    benchmark_test_case<boost::multi_array<float, 2>>(cfg);
  } else if (test_case == "float:unaligned:single") {
    benchmark_test_case<std::vector<float>>(cfg);
  } else if (test_case == "double:unaligned:many") {
    benchmark_test_case<boost::multi_array<double, 2>>(cfg);
  } else if (test_case == "double:unaligned:single") {
    benchmark_test_case<std::vector<double>>(cfg);
  } else if (test_case == "complex:float:aligned:many") {
    benchmark_test_case<jb::fftw::aligned_multi_array<std::complex<float>, 2>>(
        cfg);
  } else if (test_case == "complex:float:aligned:single") {
    benchmark_test_case<jb::fftw::aligned_vector<std::complex<float>>>(cfg);
  } else if (test_case == "complex:double:aligned:many") {
    benchmark_test_case<jb::fftw::aligned_multi_array<std::complex<double>, 2>>(
        cfg);
  } else if (test_case == "complex:double:aligned:single") {
    benchmark_test_case<jb::fftw::aligned_vector<std::complex<double>>>(cfg);
  } else if (test_case == "complex:float:unaligned:many") {
    benchmark_test_case<boost::multi_array<std::complex<float>, 2>>(cfg);
  } else if (test_case == "complex:float:unaligned:single") {
    benchmark_test_case<std::vector<std::complex<float>>>(cfg);
  } else if (test_case == "complex:double:unaligned:many") {
    benchmark_test_case<boost::multi_array<std::complex<double>, 2>>(cfg);
  } else if (test_case == "complex:double:unaligned:single") {
    benchmark_test_case<std::vector<std::complex<double>>>(cfg);
  } else {
    std::ostringstream os;
    os << "Unknown test case (" << test_case << ")" << std::endl;
    os << " --test-case must be one of"
       << ": float:aligned:many"
       << ", double:aligned:many"
       << ", float:unaligned:many"
       << ", double:aligned:many"
       << ", complex:float:aligned:many"
       << ", complex:double:aligned:many"
       << ", complex:float:unaligned:many"
       << ", complex:double:unaligned:many"
       << ", float:aligned:single"
       << ", double:aligned:single"
       << ", float:unaligned:single"
       << ", double:aligned:single"
       << ", complex:float:aligned:single"
       << ", complex:double:aligned:single"
       << ", complex:float:unaligned:single"
       << ", complex:double:unaligned:single" << std::endl;
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

namespace {
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
  if (n_timeseries() < 1) {
    std::ostringstream os;
    os << "n_timeseries must be > 0 (" << n_timeseries() << ")";
    throw jb::usage(os.str(), 1);
  }
}

} // anonymous namespace
