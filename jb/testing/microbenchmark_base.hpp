#ifndef jb_testing_microbenchmark_base_hpp
#define jb_testing_microbenchmark_base_hpp

#include <jb/testing/microbenchmark_config.hpp>

#include <chrono>
#include <iostream>
#include <utility>
#include <vector>

namespace jb {
namespace testing {

/**
 * Refactor non-template parts of the microbenchmark template class.
 */
class microbenchmark_base {
public:
  typedef std::chrono::steady_clock clock;
  typedef typename clock::duration duration;
  typedef typename std::pair<int, duration> result;
  typedef typename std::vector<result> results;

  /**
   * Constructor from a configuration
   */
  explicit microbenchmark_base(microbenchmark_config const& cfg)
      : config_(cfg) {
  }

  /**
   * A simple object to contain the summary of the test results
   */
  struct summary {
    summary()
        : min(0)
        , p25(0)
        , p50(0)
        , p75(0)
        , p90(0)
        , p99(0)
        , p99_9(0)
        , max(0)
        , n(0) {
    }
    explicit summary(results const& arg);

    duration min;
    duration p25;
    duration p50;
    duration p75;
    duration p90;
    duration p99;
    duration p99_9;
    duration max;
    std::size_t n;
  };

  /**
   * Produce the results of the test in a format that works for most cases
   *
   * In most of the JayBeams tests we print the summary results to
   * stderr and the summary results to stdout.  That makes it
   * relatively easy to capture both separately in driver scripts.
   */
  void typical_output(results const& r) const;

  /// Stream the detailed results
  void write_results(std::ostream& os, results const& r) const;

protected:
  microbenchmark_config config_;
};

/// Stream the summary of a microbenchmark results in microseconds
std::ostream&
operator<<(std::ostream& os, microbenchmark_base::summary const& x);

} // namespace testing
} // namespace jb

#endif // jb_testing_microbenchmark_base_hpp
