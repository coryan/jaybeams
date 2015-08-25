#ifndef jb_histogram_summary_hpp
#define jb_histogram_summary_hpp

#include <iosfwd>

namespace jb {

/// A simple class to capture summary information about a histogram
struct histogram_summary {
  double min;
  double p25;
  double p50;
  double p75;
  double p90;
  double p99;
  double max;
  std::size_t nsamples;
};

std::ostream& operator<<(std::ostream& os, histogram_summary const& x);

} // namespace jb

#endif // jb_histogram_summary_hpp
