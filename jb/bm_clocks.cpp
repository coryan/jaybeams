#include <jb/testing/microbenchmark.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <stdexcept>

/**
 * Convenience types and functions for benchmarking std::chrono clocks.
 */
namespace {

/**
 * Base class for the std::chrono clock wrapper.
 */
class wrapped_clock_base {
public:
  virtual ~wrapped_clock_base() {
  }

  virtual void run() const = 0;
};

/**
 * The fixture tested by the microbenchmark.
 */
class fixture {
public:
  fixture(std::string const& clock_name);
  fixture(int size, std::string const& clock_name);

  void run();

private:
  std::unique_ptr<wrapped_clock_base> wrapped_clock_;
};

typedef jb::testing::microbenchmark<fixture> benchmark;

} // anonymous namespace

int main(int argc, char* argv[]) try {
  jb::testing::microbenchmark_config cfg;
  cfg.test_case("std::chrono::steady_clock").process_cmdline(argc, argv);

  std::cout << "Configuration for test\n" << cfg << std::endl;

  benchmark bm(cfg);
  auto r = bm.run(cfg.test_case());

  benchmark::summary s(r);
  std::cerr << cfg.test_case() << " summary " << s << std::endl;
  if (cfg.verbose()) {
    bm.write_results(std::cout, r);
  }

  return 0;
} catch (jb::usage const& ex) {
  std::cerr << "usage: " << ex.what() << std::endl;
  return 1;
} catch (std::exception const& ex) {
  std::cerr << "standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "unknown exception raised" << std::endl;
  return 1;
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
 * Wrap a std::chrono class in a polymorphic class.
 */
template <typename clock> class wrapped_clock : public wrapped_clock_base {
public:
  wrapped_clock(int calls_per_iteration)
      : calls_per_iteration_(calls_per_iteration) {
  }

  virtual void run() const override {
    for (int i = 0; i != calls_per_iteration_; ++i) {
      clock::now();
    }
  }

private:
  int calls_per_iteration_;
};

static std::uint64_t read_rdtscp() {
  std::uint64_t hi, lo;
  std::uint32_t aux;
  __asm__ __volatile__("rdtscp\n" : "=a"(lo), "=d"(hi), "=c"(aux) : :);
  return (hi << 32) + lo;
}

class wrapped_rdtscp : public wrapped_clock_base {
public:
  wrapped_rdtscp(int calls_per_iteration)
      : calls_per_iteration_(calls_per_iteration) {
  }

  virtual void run() const override {
    for (int i = 0; i != calls_per_iteration_; ++i) {
      (void)read_rdtscp();
    }
  }

private:
  int calls_per_iteration_;
};

fixture::fixture(std::string const& clock_name)
    : fixture(defaults::clock_repetitions, clock_name) {
}

fixture::fixture(int size, std::string const& clock_name) {
  using namespace std::chrono;
  if (clock_name == "std::chrono::steady_clock") {
    wrapped_clock_.reset(new wrapped_clock<steady_clock>(size));
  } else if (clock_name == "std::chrono::high_resolution_clock") {
    wrapped_clock_.reset(new wrapped_clock<high_resolution_clock>(size));
  } else if (clock_name == "std::chrono::system_clock") {
    wrapped_clock_.reset(new wrapped_clock<system_clock>(size));
  } else if (clock_name == "rdtscp") {
    wrapped_clock_.reset(new wrapped_rdtscp(size));
  } else {
    std::ostringstream os;
    os << "unknown value for --clock-name (" << clock_name << ")\n";
    os << "value must be one of"
       << ": std::chrono::steady_clock"
       << ", std::chrono::high_resolution_clock"
       << ", std::chrono::system_clock"
       << ", rdtscp";
    throw jb::usage(os.str(), 1);
  }
}

void fixture::run() {
  wrapped_clock_->run();
}

} // anonymous namespace
