#include <jb/pitch2/add_order_message.hpp>
#include <jb/pitch2/base_add_order_message_streaming.hpp>

namespace jb {
namespace pitch2 {

std::ostream& operator<<(std::ostream& os, add_order_message const& x) {
  using base = base_add_order_message<
      add_order_message::quantity_type, add_order_message::symbol_type,
      add_order_message::price_type>;
  return os << static_cast<base const&>(x);
}

} // namespace pitch2
} // namespace jb
