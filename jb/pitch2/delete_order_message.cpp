#include <jb/pitch2/delete_order_message.hpp>

#include <ostream>

namespace jb {
namespace pitch2 {

std::ostream& operator<<(std::ostream& os, delete_order_message const& x) {
  return os << "length=" << static_cast<int>(x.length.value())
            << ",message_type=" << static_cast<int>(x.message_type.value())
            << ",time_offset=" << x.time_offset.value()
            << ",order_id=" << x.order_id.value();
}

} // namespace pitch2
} // namespace jb
