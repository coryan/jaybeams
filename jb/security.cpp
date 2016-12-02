#include "jb/security.hpp"

#include <sstream>
#include <stdexcept>

namespace jb {

security::security()
    : directory_()
    , id_()
    , generation_()
    , entry_() {
}

void security::check_directory(char const* function_name) const {
  if (directory_) {
    return;
  }
  std::ostringstream os;
  os << "undefined directory for security in " << function_name;
  throw std::runtime_error(os.str());
}

} // namespace jb
