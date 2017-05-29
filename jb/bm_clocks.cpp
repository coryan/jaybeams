#include <jb/testing/microbenchmark.hpp>
#include <jb/testing/microbenchmark_group_main.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>

/**
 * Convenience types and functions for benchmarking std::chrono clocks.
 */
namespace {
using config = jb::testing::microbenchmark_config;
jb::testing::microbenchmark_group<config> create_testcases();
} // anonymous namespace

int main(int argc, char* argv[]) {
  auto testcases = create_testcases();
  return jb::testing::microbenchmark_group_main(argc, argv, testcases);
}

namespace {
/**
 * @namespace defaults
 *
 * Define defaults for program parameters.
 */
namespace defaults {

#ifndef JB_DEFAULTS_clock_repetitions
#define JB_DEFAULTS_clock_repetitions 1000
#endif // JB_DEFAULTS_clock_repetitions

int clock_repetitions = JB_DEFAULTS_clock_repetitions;
} // namespace defaults

/**
 * The fixture tested by the microbenchmark.
 *
 * @tparam clock_type the type of clock tested
 */
template <typename clock_type>
class fixture {
public:
  fixture()
      : fixture(defaults::clock_repetitions) {
  }

  explicit fixture(int size)
      : calls_per_iteration_(size) {
  }

  int run() {
    for (int i = 0; i != calls_per_iteration_; ++i) {
      (void)clock_type::now();
    }
    return calls_per_iteration_;
  }

private:
  int calls_per_iteration_;
};

/// Fake a std::chrono clock using rdtscp
struct wrapped_rtdscp {
  static std::uint64_t now() {
    std::uint64_t hi, lo;
    std::uint32_t aux;
    __asm__ __volatile__("rdtscp\n" : "=a"(lo), "=d"(hi), "=c"(aux) : :);
    return (hi << 32) + lo;
  }
};

/// Fake a std::chrono clock using rdtsc
struct wrapped_rtdsc {
  static std::uint64_t now() {
    std::uint64_t hi, lo;
    std::uint32_t aux;
    __asm__ __volatile__("rdtsc\n" : "=a"(lo), "=d"(hi), "=c"(aux) : :);
    return (hi << 32) + lo;
  }
};

template <typename clock_type>
std::function<void(config const&)> test_case() {
  return [](config const& cfg) {
    using benchmark = jb::testing::microbenchmark<fixture<clock_type>>;
    benchmark bm(cfg);
    auto r = bm.run();
    bm.typical_output(r);
  };
}

jb::testing::microbenchmark_group<config> create_testcases() {
  using namespace std::chrono;
  return jb::testing::microbenchmark_group<config>{
      {"std::chrono::steady_clock", test_case<steady_clock>()},
      {"std::chrono::high_resolution_clock",
       test_case<high_resolution_clock>()},
      {"std::chrono::system_clock_clock", test_case<system_clock>()},
      {"rdtscp", test_case<wrapped_rtdscp>()},
      {"rdtsc", test_case<wrapped_rtdsc>()},
  };
}

} // anonymous namespace
