#include <jb/pitch2/time_message.hpp>

#include <ostream>

namespace jb {
namespace pitch2 {

std::ostream& operator<<(std::ostream& os, time_message const& x) {
  return os << "length=" << static_cast<int>(x.length.value())
            << ",message_type=" << static_cast<int>(x.message_type.value())
            << ",time=" << x.time.value();
}

} // namespace pitch2
} // namespace jb
