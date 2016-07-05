#include <jb/itch5/check_offset.hpp>

#include <sstream>
#include <stdexcept>

template<>
void jb::itch5::check_offset<true>(
    char const* msg, std::size_t size, std::size_t offset, std::size_t n) {
  if (0 < n and offset < size and offset + n <= size) {
    return;
  }
  std::ostringstream os;
  os << "invalid offset or field length for buffer in " << msg
     << " size=" << size << ", offset=" << offset << ", n=" << n;
  throw std::runtime_error(os.str());
}

void jb::itch5::raise_validation_failed(
    char const* where, char const* what) {
  std::ostringstream os;
  os << "message or field validation failed in " << where
     << ": " << what;
  throw std::runtime_error(os.str());
}
