#include <jb/fftw/time_delay_estimator.hpp>
#include <jb/testing/create_square_timeseries.hpp>
#include <jb/testing/delay_timeseries.hpp>
#include <jb/testing/microbenchmark.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <stdexcept>

/**
 * Convenience types and functions for benchmarking std::chrono clocks.
 */
namespace {

// We expect delays to be around 1250 microseconds, max delays are
// around 6,000, so we need at least a 3*6000 ~ 18,000 microsecond
// baseline.  Rounded up to a power of 2 that is 32768.  Sampling
// every 16 microseconds that is 32768 / 16.
std::chrono::microseconds const expected_delay(1250);
std::chrono::microseconds const sampling_period(16);
int nsamples = 32768 / 16;

class config : public jb::config_object {
 public:
  config();
  config_object_constructors(config);

  jb::config_attribute<config,jb::testing::microbenchmark_config> benchmark;
  jb::config_attribute<config,std::string> test_case;
};

template<typename timeseries_type>
class fixture {
 public:
  fixture() : fixture(nsamples) {}
  fixture(int size)
      : a(size)
      , b(size)
      , estimator(a, b) {
    jb::testing::create_square_timeseries(size, a);
    b = jb::testing::delay_timeseries_periodic(
        a, expected_delay, sampling_period);
  }

  void run() {
    auto e = estimator.estimate_delay(a, b);
    if (not e.first) throw std::runtime_error("estimation failed");
  }

  typedef jb::fftw::time_delay_estimator<timeseries_type> tested;
 private:
  timeseries_type a;
  timeseries_type b;
  tested estimator;
};

template<typename vector_type>
void benchmark_test_case(config const& cfg) {
  typedef jb::testing::microbenchmark<fixture<vector_type>> benchmark;
  benchmark bm(cfg.benchmark());

  auto r = bm.run();
  typename benchmark::summary s(r);
  std::cerr << cfg.test_case() << " summary " << s << std::endl;
  if (cfg.benchmark().verbose()) {
    bm.write_results(std::cout, r);
  }
}

} // anonymous namespace

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.process_cmdline(argc, argv);

  std::cout << "Configuration for test\n" << cfg << std::endl;

  if (cfg.test_case() == "float:aligned") {
    benchmark_test_case<jb::fftw::aligned_vector<float>>(cfg);
  } else if (cfg.test_case() == "double:aligned") {
    benchmark_test_case<jb::fftw::aligned_vector<double>>(cfg);
  } else if (cfg.test_case() == "float:unaligned") {
    benchmark_test_case<std::vector<float>>(cfg);
  } else if (cfg.test_case() == "double:unaligned") {
    benchmark_test_case<std::vector<double>>(cfg);
  } else if (cfg.test_case() == "complex:float:aligned") {
    benchmark_test_case<jb::fftw::aligned_vector<std::complex<float>>>(cfg);
  } else if (cfg.test_case() == "complex:double:aligned") {
    benchmark_test_case<jb::fftw::aligned_vector<std::complex<double>>>(cfg);
  } else if (cfg.test_case() == "complex:float:unaligned") {
    benchmark_test_case<std::vector<std::complex<float>>>(cfg);
  } else if (cfg.test_case() == "complex:double:unaligned") {
    benchmark_test_case<std::vector<std::complex<double>>>(cfg);
  } else {
    std::cerr << "Unknown test case (" << cfg.test_case() << ")" << std::endl;
    return 1;
  }

  return 0;
} catch(jb::usage const& ex) {
  std::cerr << "usage: " << ex.what() << std::endl;
  return 1;
} catch(std::exception const& ex) {
  std::cerr << "standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch(...) {
  std::cerr << "unknown exception raised" << std::endl;
  return 1;
}

namespace {
namespace defaults {

#ifndef JB_DEFAULTS_test_case
#define JB_DEFAULTS_test_case "float:aligned"
#endif // JB_DEFAULTS_test_case

std::string test_case = JB_DEFAULTS_test_case;
} // namespace defaults

config::config()
    : benchmark(
        desc("benchmark")
        .help("Benchmark Parameters"), this)
    , test_case(
        desc("test-case")
        .help("The name of the test case, must be one of the following options"
              ": float:aligned"
              ", double:aligned"
              ", float:unaligned"
              ", double:aligned"
              ", complex:float:aligned"
              ", complex:double:aligned"
              ", complex:float:unaligned"
              ", complex:double:unaligned"
              ),
        this, defaults::test_case) {
}

} // anonymous namespace
