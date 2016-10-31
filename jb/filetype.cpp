#include "jb/filetype.hpp"

bool jb::is_gz(std::string const& filename) {
  char const ext[] = ".gz";
  constexpr std::size_t elen = sizeof(ext) - 1;
  auto len = filename.length();
  if (filename.length() <= elen) {
    return false;
  }
  return 0 == filename.compare(len - elen, elen, ext);
}
