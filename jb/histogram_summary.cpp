#include <jb/histogram_summary.hpp>

#include <iostream>

std::ostream& jb::operator<<(std::ostream& os, histogram_summary const& x) {
  if (x.nsamples == 0) {
    return os << "no samples recorded";
  }
  return os << "nsamples=" << x.nsamples
            << ", min=" << x.min
            << ", p25=" << x.p25
            << ", p50=" << x.p50
            << ", p75=" << x.p75
            << ", p90=" << x.p90
            << ", p99=" << x.p99
            << ", max=" << x.max;
}

