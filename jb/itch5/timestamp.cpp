#include "jb/itch5/timestamp.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>

template <>
void jb::itch5::check_timestamp_range<true>(timestamp const& t) {
  auto full_day = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::seconds(24 * 3600));

  if (t.ts < full_day) {
    return;
  }
  std::ostringstream os;
  os << "out of range timestamp <" << t.ts.count() << "> expected value in [0,"
     << full_day.count() << ") range";
  throw std::range_error(os.str());
}

std::ostream& jb::itch5::operator<<(std::ostream& os, timestamp const& x) {
  auto sec = std::chrono::duration_cast<std::chrono::seconds>(x.ts);
  auto nn = (x.ts - sec).count();

  auto t = sec.count();
  auto ss = t % 60;
  t /= 60;
  auto mm = t % 60;
  t /= 60;
  auto hh = t;
  return os << std::setw(2) << std::setfill('0') << hh << std::setw(2)
            << std::setfill('0') << mm << std::setw(2) << std::setfill('0')
            << ss << "." << std::setw(9) << std::setfill('0') << nn;
}
