#include <jb/pitch2/unit_clear_message.hpp>

#include <ostream>

namespace jb {
namespace pitch2 {

std::ostream& operator<<(std::ostream& os, unit_clear_message const& x) {
  return os << "length=" << static_cast<int>(x.length.value())
            << ",message_type=" << static_cast<int>(x.message_type.value())
            << ",time_offset=" << x.time_offset.value();
}

} // namespace pitch2
} // namespace jb

