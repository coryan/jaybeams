#include <jb/testing/microbenchmark_base.hpp>

#include <algorithm>

void jb::testing::microbenchmark_base::typical_output(results const& r) const {
  summary s(r);
  if (config_.test_case() != "") {
    std::cerr << config_.test_case() << " ";
  }
  std::cerr << "summary " << s << std::endl;
  if (config_.verbose()) {
    write_results(std::cout, r);
  }
}

void jb::testing::microbenchmark_base::write_results(
    std::ostream& os, results const& r) const {
  using namespace std::chrono;
  for(auto const & v : r) {
    os << config_.prefix()
       << duration_cast<nanoseconds>(v.second).count() << "\n";
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
  // Print the summary results in microseconds because they are often
  return os << "min=" << duration_cast<microseconds>(x.min).count()
            << "us, p25=" << duration_cast<microseconds>(x.p25).count()
            << "us, p50=" << duration_cast<microseconds>(x.p50).count()
            << "us, p75=" << duration_cast<microseconds>(x.p75).count()
            << "us, p90=" << duration_cast<microseconds>(x.p90).count()
            << "us, p99=" << duration_cast<microseconds>(x.p99).count()
            << "us, p99.9=" << duration_cast<microseconds>(x.p99_9).count()
            << "us, max=" << duration_cast<microseconds>(x.max).count()
            << "us, N=" << x.n;
}
