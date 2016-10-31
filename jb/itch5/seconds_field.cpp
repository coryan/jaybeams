#include "jb/itch5/seconds_field.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>

template <>
void jb::itch5::check_seconds_field_range<true>(seconds_field const& t) {
  auto full_day = std::chrono::seconds(24 * 3600);

  if (t.seconds() < full_day) {
    return;
  }
  std::ostringstream os;
  os << "out of range seconds_field <" << t.seconds().count()
     << "> expected value in [0," << full_day.count() << ") range";
  throw std::range_error(os.str());
}

std::ostream& jb::itch5::operator<<(std::ostream& os, seconds_field const& x) {
  auto t = x.int_seconds();

  auto ss = t % 60;
  t /= 60;
  auto mm = t % 60;
  t /= 60;
  auto hh = t;
  return os << std::setw(2) << std::setfill('0') << hh << ':' << std::setw(2)
            << std::setfill('0') << mm << ':' << std::setw(2)
            << std::setfill('0') << ss;
}
