#include "jb/assert_throw.hpp"

#include <sstream>
#include <stdexcept>

void jb::raise_assertion_failure(char const* filename, int lineno,
                                 char const* predicate) {
  std::ostringstream os;
  os << "assertion failed in " << filename << ":" << lineno << " - "
     << predicate;
  throw std::runtime_error(os.str());
}
