#ifndef jb_testing_microbenchmark_group_main_hpp
#define jb_testing_microbenchmark_group_main_hpp

#include <jb/testing/microbenchmark_config.hpp>
#include <jb/log.hpp>

#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace jb {
namespace testing {

/**
 * Define a representation for a group of microbenchmarks.
 *
 * Each microbenchmark is encapsulated in a (type erased)
 * std::function, which typically creates an instance of
 * jb::testing::microbenchmark and runs it.  The keys into the group
 * are the names of the testcases.
 *
 * @tparam config the configuration class for the group of
 * microbenchmarks.
 */
template <typename config>
using microbenchmark_group =
    std::map<std::string, std::function<void(config const& cfg)>>;

/**
 * Implement the main() function for a multi-testcase benchmark.
 *
 * A common idiom in complex benchmarks is to create multiple
 * testcases that exercise different implementations, different
 * template instantiations, or otherwise vary the test in a
 * non-trivial way.
 *
 * In such cases the main() function follows a well understood
 * pattern:
 *   - Initialize the benchmark configuration.
 *   - Find which testcase is requested.
 *   - Call a function for that testcase.
 *
 * This class refactors that pattern into a static function.
 *
 * @tparam config the configuration object used by the benchmark.  We
 * expect that it has at least two configuration attributes:
 *  - log: of type jb::log::config
 *  - microbenchmark: of type jb::testing::microbenchmark_config
 *
 * @return the exit status for the program
 * @param argc the number of arguments in @a argv
 * @param argv the command-line arguments
 * @param testcases a group of microbenchmarks, executes the one
 * selected via the --microbenchmark.test-case command-line
 * argument.
 * @throws nothing, all exceptions are captured and printed to
 * stderr before the program exits.
 */
template <typename config>
int microbenchmark_group_main(
    int argc, char* argv[], microbenchmark_group<config> const& testcases) try {
  // Create a default instance of the configuration ...
  config cfg;
  // ... parse the command-line arguments ...
  cfg.process_cmdline(argc, argv);
  // initialize the logging framework ...
  jb::log::init(cfg.log());
  // ... get the microbenchmark configuration ..
  jb::testing::microbenchmark_config const& bmcfg = cfg.microbenchmark();
  // ... print out the test parameters ...
  if (bmcfg.verbose()) {
    JB_LOG(info) << "Configuration for test\n" << cfg << "\n";
  }
  // ... find out if the requested test case is in the list of known
  // test cases ...
  auto testcase = testcases.find(bmcfg.test_case());
  // ... if not, print a usage message and terminate the test ...
  if (testcase == testcases.end()) {
    std::ostringstream os;
    os << "Unknown test case (" << bmcfg.test_case() << ")\n";
    os << " --microbenchmark.test-case must be one of:";
    for (auto const& i : testcases) {
      os << "  " << i.first << "\n";
    }
    throw jb::usage(os.str(), 1);
  }
  // ... run the test that was configured ...
  testcase->second(cfg);
  // ... exit with a success status ...
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

} // namespace testing
} // namespace jb

#endif // jb_testing_microbenchmark_group_main_hpp
