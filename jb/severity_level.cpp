#include <jb/severity_level.hpp>

#include <algorithm>
#include <cstring>
#include <sstream>
#include <stdexcept>

namespace {
/// The names for the security levels
char const* const severity_level_names[] = {
  "TRACE",
  "DEBUG",
  "INFO",
  "NOTICE",
  "WARNING",
  "ERROR",
  "CRITICAL",
  "ALERT",
  "FATAL"
};

/// Compute the width for the security level field.
int max_severity_level_width() {
  std::size_t r = 0;
  for (char const* name : severity_level_names) {
    r = std::max(r, std::strlen(name));
  }
  return r;
}

/// Store the width in a constant
int const sev_level_width = max_severity_level_width();
} // anonymous namespace

std::ostream& jb::operator<<(std::ostream& os, severity_level const& x) {
  std::size_t const count =
      sizeof(severity_level_names)/sizeof(severity_level_names[0]);
  int idx = static_cast<int>(x);
  if (idx < 0 or std::size_t(idx) >= count) {
    return os << "[invalid severity (" << idx << ")]";
  }
  return os << severity_level_names[idx];
}

std::istream& jb::operator>>(std::istream& is, severity_level& x) {
  std::string name;
  is >> name;
  parse(x, name);
  return is;
}

void jb::parse(severity_level& lhs, std::string const& rhs) {
  std::size_t const count =
      sizeof(severity_level_names)/sizeof(severity_level_names[0]);
  for (std::size_t i = 0; i != count; ++i) {
    if (rhs == severity_level_names[i]) {
      lhs = static_cast<jb::severity_level>(i);
      return;
    }
  }
  std::ostringstream os;
  os << "Unknown or invalid severity level (" << rhs << ")";
  throw std::invalid_argument(os.str());
}

char const* jb::get_name(severity_level const& rhs) {
  std::size_t const count =
      sizeof(severity_level_names)/sizeof(severity_level_names[0]);
  if (0 <= static_cast<int>(rhs) and static_cast<std::size_t>(rhs) < count) {
    return severity_level_names[static_cast<int>(rhs)];
  }
  std::ostringstream os;
  os << "Unknown or invalid severity level (" << rhs << ")";
  throw std::invalid_argument(os.str());
}

int jb::severity_level_width() {
  return sev_level_width;
}
