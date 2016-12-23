#include "jb/detail/os_error.hpp"

#include <system_error>

void jb::detail::os_check_error(int r, char const* msg) {
  if (r != -1) {
    return;
  }
  throw std::system_error(errno, std::system_category(), msg);
}
