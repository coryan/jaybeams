#include <jb/detail/array_traits.hpp>
#include <jb/fftw/time_delay_estimator_many.hpp>
#include <jb/testing/create_triangle_timeseries.hpp>
#include <jb/testing/delay_timeseries.hpp>
#include <jb/testing/microbenchmark.hpp>
#include <jb/testing/tde_square_sum.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

// We expect delays to be around 1250 microseconds, max delays are
// around 6,000, so we need at least a 3*6000 ~ 18,000 microsecond
// baseline.  Rounded up to a power of 2 that is 32768.  Sampling
// every 16 microseconds that is 32768 / 16.
std::chrono::microseconds const expected_delay(1250);
std::chrono::microseconds const sampling_period(16);
int nsamples = 32768 / 16;

/**
 * Fixture for container like used when timeseries = 1 (default value).
 *
 * Timeseries value is ignored.
 *
 * @tparam timeseries_type container (e.g. std::vector<>) containing a
 * timeseries
 */
template <typename timeseries_type>
class fixture {
public:
  fixture(int timeseries)
      : fixture(nsamples, timeseries) {
  }
  fixture(int size, int timeseries)
      : a_(size)
      , b_(size)
      , estimator_(a_, b_)
      , confidence_(a_)
      , tde_(a_)
      , sum2_(a_) {
    jb::testing::create_triangle_timeseries(size, a_);
    b_ = jb::testing::delay_timeseries_periodic(
        a_, expected_delay, sampling_period);
    sum2_ = jb::testing::sum_square(a_);
  }

  void run() {
    estimator_.estimate_delay(confidence_, tde_, a_, b_, sum2_);
  }

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
 * Fixture for boost multi array, only 2 dimensions allowed.
 *
 * @tparam T value type stores on the multi array
 * @tparam A Allocator type to allocate value types
 */
template <typename T, typename A>
class fixture<boost::multi_array<T, 2, A>> {
public:
  fixture(int timeseries)
      : fixture(nsamples, timeseries) {
  }
  fixture(int size, int timeseries)
      : a_(boost::extents[timeseries][size])
      , b_(boost::extents[timeseries][size])
      , estimator_(a_, b_)
      , confidence_(a_)
      , tde_(a_)
      , sum2_(a_) {
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
void benchmark_test_case(jb::testing::microbenchmark_config const& cfg) {
  using benchmark = jb::testing::microbenchmark<fixture<vector_type>>;
  benchmark bm(cfg);
  auto r = bm.run(cfg.timeseries());
  bm.typical_output(r);
}

} // anonymous namespace

int main(int argc, char* argv[]) try {
  jb::testing::microbenchmark_config cfg;
  cfg.test_case("float:aligned").process_cmdline(argc, argv);

  std::cout << "Configuration for test\n" << cfg << std::endl;

  // check if it is 2 dimensional
  bool dim_2 = cfg.timeseries() > 1;

  if (cfg.test_case() == "float:aligned") {
    if (dim_2) {
      benchmark_test_case<jb::fftw::aligned_multi_array<float, 2>>(cfg);
    } else {
      benchmark_test_case<jb::fftw::aligned_vector<float>>(cfg);
    }
  } else if (cfg.test_case() == "double:aligned") {
    if (dim_2) {
      benchmark_test_case<jb::fftw::aligned_multi_array<double, 2>>(cfg);
    } else {
      benchmark_test_case<jb::fftw::aligned_vector<double>>(cfg);
    }
  } else if (cfg.test_case() == "float:unaligned") {
    if (dim_2) {
      benchmark_test_case<boost::multi_array<float, 2>>(cfg);
    } else {
      benchmark_test_case<std::vector<float>>(cfg);
    }
  } else if (cfg.test_case() == "double:unaligned") {
    if (dim_2) {
      benchmark_test_case<boost::multi_array<double, 2>>(cfg);
    } else {
      benchmark_test_case<std::vector<double>>(cfg);
    }
  } else if (cfg.test_case() == "complex:float:aligned") {
    if (dim_2) {
      benchmark_test_case<
          jb::fftw::aligned_multi_array<std::complex<float>, 2>>(cfg);
    } else {
      benchmark_test_case<jb::fftw::aligned_vector<std::complex<float>>>(cfg);
    }
  } else if (cfg.test_case() == "complex:double:aligned") {
    if (dim_2) {
      benchmark_test_case<
          jb::fftw::aligned_multi_array<std::complex<double>, 2>>(cfg);
    } else {
      benchmark_test_case<jb::fftw::aligned_vector<std::complex<double>>>(cfg);
    }
  } else if (cfg.test_case() == "complex:float:unaligned") {
    if (dim_2) {
      benchmark_test_case<boost::multi_array<std::complex<float>, 2>>(cfg);
    } else {
      benchmark_test_case<std::vector<std::complex<float>>>(cfg);
    }
  } else if (cfg.test_case() == "complex:double:unaligned") {
    if (dim_2) {
      benchmark_test_case<boost::multi_array<std::complex<double>, 2>>(cfg);
    } else {
      benchmark_test_case<std::vector<std::complex<double>>>(cfg);
    }
  } else {
    std::ostringstream os;
    os << "Unknown test case (" << cfg.test_case() << ")" << std::endl;
    os << " --test-case must be one of"
       << ": float:aligned"
       << ", double:aligned"
       << ", float:unaligned"
       << ", double:aligned"
       << ", complex:float:aligned"
       << ", complex:double:aligned"
       << ", complex:float:unaligned"
       << ", complex:double:unaligned" << std::endl;
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
