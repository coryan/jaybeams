#include "jb/as_hhmmss.hpp"

#include <boost/io/ios_state.hpp>

#include <iomanip>
#include <iostream>

std::ostream& jb::operator<<(std::ostream& os, jb::as_hhmmssu const& x) {
  boost::io::ios_flags_saver saver(os);
  auto usec = x.t.count() % 1000000;
  return os << jb::as_hhmmss(x.t) << '.' << std::setw(6) << std::setfill('0')
            << usec;
}

std::ostream& jb::operator<<(std::ostream& os, jb::as_hhmmss const& x) {
  boost::io::ios_flags_saver saver(os);
  auto t = x.t.count() / 1000000;
  auto ss = t % 60;
  t /= 60;
  auto mm = t % 60;
  t /= 60;
  auto hh = t;
  return os << std::setw(2) << std::setfill('0') << hh << std::setw(2)
            << std::setfill('0') << mm << std::setw(2) << std::setfill('0')
            << ss;
}

std::ostream& jb::operator<<(std::ostream& os, jb::as_hh_mm_ss_u const& x) {
  boost::io::ios_flags_saver saver(os);
  auto usec = x.t.count() % 1000000;
  auto t = x.t.count() / 1000000;
  auto ss = t % 60;
  t /= 60;
  auto mm = t % 60;
  t /= 60;
  auto hh = t;
  return os << std::setw(2) << std::setfill('0') << hh << ":" << std::setw(2)
            << std::setfill('0') << mm << ":" << std::setw(2)
            << std::setfill('0') << ss << "." << std::setw(6)
            << std::setfill('0') << usec;
}
