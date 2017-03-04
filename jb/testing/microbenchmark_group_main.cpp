#include "jb/testing/microbenchmark_group_main.hpp"

namespace jb {
namespace testing {

int microbenchmark_group_main(
    int argc, char* argv[],
    microbenchmark_group<microbenchmark_config> const& testcases) try {
  // Create a default instance of the configuration ...
  microbenchmark_config cfg;
  // ... parse the command-line arguments ...
  cfg.process_cmdline(argc, argv);
  // ... print out the test parameters ...
  if (cfg.verbose()) {
    JB_LOG(info) << "Configuration for test\n" << cfg << "\n";
  }
  // ... find out if the requested test case is in the list of known
  // test cases ...
  auto testcase = testcases.find(cfg.test_case());
  // ... if not, print a usage message and terminate the test ...
  if (testcase == testcases.end()) {
    std::ostringstream os;
    os << "Unknown test case (" << cfg.test_case() << ")\n";
    os << " --test-case must be one of:";
    for (auto const& i : testcases) {
      os << "  " << i.first << "\n";
    }
    throw jb::usage(os.str(), 1);
  }
  // ... run the test that was configured ...
  testcase->second(cfg);
  // ... exit with a success status ...
  return 0;
} catch (...) {
  return detail::report_exception_at_exit();
}

namespace detail {
int report_exception_at_exit() try {
  // rethrow the current exception so we can obtain its type in the
  // catch() blocks ...
  throw;
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

} // namespace detail

} // namespace testing
} // namespace jb
