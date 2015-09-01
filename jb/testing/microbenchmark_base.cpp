#include <jb/testing/microbenchmark_base.hpp>

#include <algorithm>

void jb::testing::microbenchmark_base::write_results(
    std::ostream& os, results const& r) const {
  using namespace std::chrono;
  for(auto const & v : r) {
    os << duration_cast<nanoseconds>(v.second).count() << "\n";
  }
}

jb::testing::microbenchmark_base::summary::summary(results const& arg) {
  results r = arg;
  auto cmp = [](result const& lhs, result const& rhs) {
    return lhs.second < rhs.second;
  };
  std::sort(r.begin(), r.end(), cmp);
  n = r.size();
  auto p = [this,&r](double p) { return r[ int(p * n / 100) ].second; };
  if (not r.empty()) {
    min = p(0);
    p25 = p(25);
    p50 = p(50);
    p75 = p(75);
    p90 = p(90);
    p99 = p(99);
    p99_9 = p(99.9);
    max = r.back().second;
  }
}

std::ostream& jb::testing::operator<<(
    std::ostream& os, jb::testing::microbenchmark_base::summary const& x) {
  using namespace std::chrono;
  return os << "min=" << duration_cast<microseconds>(x.min).count()
            << ", p25=" << duration_cast<microseconds>(x.p25).count()
            << ", p50=" << duration_cast<microseconds>(x.p50).count()
            << ", p75=" << duration_cast<microseconds>(x.p75).count()
            << ", p90=" << duration_cast<microseconds>(x.p90).count()
            << ", p99=" << duration_cast<microseconds>(x.p99).count()
            << ", p99.9=" << duration_cast<microseconds>(x.p99_9).count()
            << ", max=" << duration_cast<microseconds>(x.max).count()
            << ", N=" << x.n;
}
