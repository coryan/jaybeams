#include <jb/pitch2/modify_message.hpp>

#include <ostream>

namespace jb {
namespace pitch2 {

template <typename quantity_t, typename price_t>
std::ostream& stream_modify_message(
    std::ostream& os, modify_message<quantity_t, price_t> const& x) {
  return os << "length=" << static_cast<int>(x.length.value())
            << ",message_type=" << static_cast<int>(x.message_type.value())
            << ",time_offset=" << x.time_offset.value()
            << ",order_id=" << x.order_id.value()
            << ",quantity=" << x.quantity.value()
            << ",price=" << x.price.value()
            << ",modify_flags=" << static_cast<int>(x.modify_flags.value());
}

std::ostream& operator<<(std::ostream& os, modify_long_message const& x) {
  return stream_modify_message(os, x);
}

std::ostream& operator<<(std::ostream& os, modify_short_message const& x) {
  return stream_modify_message(os, x);
}

} // namespace pitch2
} // namespace jb
