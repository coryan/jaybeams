#include <jb/pitch2/add_order_message.hpp>

#include <ostream>

namespace jb {
namespace pitch2 {

std::ostream& operator<<(std::ostream& os, add_order_message const& x) {
  return os << "length=" << static_cast<int>(x.length.value())
            << ",message_type=" << static_cast<int>(x.message_type.value())
            << ",time_offset=" << x.time_offset.value()
            << ",order_id=" << x.order_id.value()
            << ",side_indicator=" << static_cast<char>(x.side_indicator.value())
            << ",quantity=" << x.quantity.value() << ",symbol=" << x.symbol
            << ",price=" << x.price.value()
            << ",add_flags=" << static_cast<int>(x.add_flags.value());
}

} // namespace pitch2
} // namespace jb
