#include <jb/testing/microbenchmark.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <stdexcept>

/**
 * Convenience types and functions for benchmarking std::chrono clocks.
 */
namespace {

class config : public jb::config_object {
 public:
  config();
  config_object_constructors(config);

  jb::config_attribute<config,jb::testing::microbenchmark_config> benchmark;
  jb::config_attribute<config,std::string> clock_name;
};

/**
 * Base class for the std::chrono clock wrapper.
 */
class wrapped_clock_base {
 public:
  virtual ~wrapped_clock_base() {}

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
  config cfg;
  cfg.process_cmdline(argc, argv);

  std::cout << "Configuration for test\n" << cfg;

  benchmark bm(cfg.benchmark());
  auto r = bm.run(cfg.clock_name());

  benchmark::summary s(r);
  std::cout << cfg.clock_name() << " summary " << s << std::endl;
  if (cfg.benchmark().verbose()) {
    bm.write_results(std::cout, r);
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

#ifndef JB_DEFAULTS_clock_name
#define JB_DEFAULTS_clock_name "std::chrono::steady_clock"
#endif // JB_DEFAULTS_clock_name

#ifndef JB_DEFAULTS_clock_repetitions
#define JB_DEFAULTS_clock_repetitions 1000
#endif // JB_DEFAULTS_clock_repetitions

std::string clock_name = JB_DEFAULTS_clock_name;
int clock_repetitions = JB_DEFAULTS_clock_repetitions;
} // namespace defaults

config::config()
    : benchmark(
        desc("benchmark")
        .help("Benchmark Parameters"), this)
    , clock_name(
        desc("clock-name")
        .help("The name of the clock, must be one of the following options"
              ": std::chrono::steady_clock"
              ", std::chrono::high_resolution_clock"
              ", or std::chrono::system_clock."),
        this, defaults::clock_name) {
}

/**
 * Wrap a std::chrono class in a polymorphic class.
 */
template<typename clock>
class wrapped_clock : public wrapped_clock_base {
 public:
  wrapped_clock(int calls_per_iteration)
      : calls_per_iteration_(calls_per_iteration)
  {}

  virtual void run() const override {
    for (int i = 0; i != calls_per_iteration_; ++i) {
      clock::now();
    }
  }

 private:
  int calls_per_iteration_;
};

fixture::fixture(std::string const& clock_name)
    : fixture(defaults::clock_repetitions, clock_name)
{}

fixture::fixture(int size, std::string const& clock_name) {
  using namespace std::chrono;
  if (clock_name == "std::chrono::steady_clock") {
    wrapped_clock_.reset(new wrapped_clock<steady_clock>(size));
  } else if (clock_name == "std::chrono::high_resolution_clock") {
    wrapped_clock_.reset(new wrapped_clock<high_resolution_clock>(size));
  } else if (clock_name == "std::chrono::system_clock") {
    wrapped_clock_.reset(new wrapped_clock<system_clock>(size));
  } else {
    std::ostringstream os;
    os << "unknown value for --clock-name, was=" << clock_name;
    throw std::invalid_argument(os.str());
  }
}

void fixture::run() {
  wrapped_clock_->run();
}

} // anonymous namespace
