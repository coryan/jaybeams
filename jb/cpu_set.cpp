#include "jb/cpu_set.hpp"
#include <jb/strtonum.hpp>

#include <iostream>
#include <sstream>
#include <stdexcept>

jb::cpu_set& jb::cpu_set::set(int cpulo, int cpuhi) {
  check_range(cpulo, cpuhi, "set");
  for (int i = cpulo; i != cpuhi + 1; ++i) {
    CPU_SET(i, &set_);
  }
  return *this;
}

jb::cpu_set& jb::cpu_set::clear(int cpulo, int cpuhi) {
  check_range(cpulo, cpuhi, "clear");
  for (int i = cpulo; i != cpuhi + 1; ++i) {
    CPU_CLR(i, &set_);
  }
  return *this;
}

jb::cpu_set jb::cpu_set::parse(std::string const& value) {
  cpu_set tmp;

  std::string element;
  for (std::istringstream is(value); is.good() and std::getline(is, element, ',');) {
    std::istringstream el(element);
    if (not el.good()) {
      parse_error(value);
    }
    std::string lo, hi;
    if (not std::getline(el, lo, '-')) {
      parse_error(value);
    }
    if (el.eof()) {
      int cpu;
      if (not jb::strtonum(lo, cpu)) {
        parse_error(value);
      }
      tmp.set(cpu);
      continue;
    }
    if (not std::getline(el, hi, '-') or not el.eof()) {
      parse_error(value);
    }
    int cpulo, cpuhi;
    if (jb::strtonum(lo, cpulo) and jb::strtonum(hi, cpuhi)) {
      tmp.set(cpulo, cpuhi);
    } else {
      parse_error(value);
    }
  }

  return tmp;
}

std::string jb::cpu_set::as_list_format() const {
  std::ostringstream os;
  char const* sep = "";
  for (int i = 0; std::size_t(i) < capacity(); ++i) {
    if (not status(i)) {
      continue;
    }
    os << sep << i;
    int end = i;
    for (; std::size_t(end) < capacity() and status(end); ++end) {
    }
    if (end != i + 1 or std::size_t(end) == capacity()) {
      i = end;
      os << "-" << end - 1;
    }
    sep = ",";
  }
  return os.str();
}

void jb::cpu_set::check_range(int cpu, char const* op) const {
  if (cpu >= 0 and std::size_t(cpu) < capacity()) {
    return;
  }
  std::ostringstream os;
  os << "cpu_set::" << op << "(" << cpu << ") - argument out of range "
     << "[0," << capacity() - 1 << "]";
  throw std::out_of_range(os.str());
}

void jb::cpu_set::check_range(int cpulo, int cpuhi, char const* op) const {
  if (cpulo < 0 or std::size_t(cpuhi) >= capacity()) {
    std::ostringstream os;
    os << "cpu_set::" << op << "(" << cpulo << "," << cpuhi << ")"
       << "- argument out of expected range [0," << capacity() - 1 << "]";
    throw std::out_of_range(os.str());
  }
}

void jb::cpu_set::parse_error(std::string const& value) {
  std::stringstream os;
  os << "cpu_set::parse() - invalid argument (" << value << ")";
  throw std::invalid_argument(os.str());
}

std::ostream& jb::operator<<(std::ostream& os, jb::cpu_set const& rhs) {
  return os << rhs.as_list_format();
}

std::istream& jb::operator>>(std::istream& is, jb::cpu_set& rhs) {
  std::string s;
  is >> s;
  jb::cpu_set tmp = jb::cpu_set::parse(s);
  rhs = std::move(tmp);
  return is;
}
