#include "jb/itch5/char_list_validator.hpp"

#include <sstream>
#include <stdexcept>

void jb::itch5::char_list_validation_failed(int x) {
  std::ostringstream os;
  os << "enum value ('" << char(x) << "'=" << x
     << ") does not match any of the expected values";
  throw std::runtime_error(os.str());
}
