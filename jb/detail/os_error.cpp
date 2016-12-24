#include "jb/detail/os_error.hpp"

#include <system_error>

void jb::detail::os_check_error(int r, char const* msg) {
  if (r == 0) {
    return;
  }
  throw std::system_error(r, std::system_category(), msg);
}
