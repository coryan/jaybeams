#ifndef jb_testing_microbenchmark_base_hpp
#define jb_testing_microbenchmark_base_hpp

#include <jb/testing/microbenchmark_config.hpp>

#include <chrono>
#include <iostream>
#include <vector>
#include <utility>

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
  microbenchmark_base(microbenchmark_config const& cfg)
      : config_(cfg) {
  }

  /// Summary of the microbenchmark results.
  struct summary {
    summary() {
    }
    summary(results const& arg);

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

  void write_results(std::ostream& os, results const& r) const;

protected:
  microbenchmark_config config_;
};

std::ostream& operator<<(std::ostream& os,
                         microbenchmark_base::summary const& x);

} // namespace testing
} // namespace jb

#endif // jb_testing_microbenchmark_base_hpp
