#include <jb/pitch2/base_add_order_message_streaming.hpp>
#include <jb/pitch2/expanded_add_order_message.hpp>

namespace jb {
namespace pitch2 {

std::ostream&
operator<<(std::ostream& os, expanded_add_order_message const& x) {
  using base = base_add_order_message<
      expanded_add_order_message::quantity_type,
      expanded_add_order_message::symbol_type,
      expanded_add_order_message::price_type>;
  return os << static_cast<base const&>(x)
            << ",participant_id=" << x.participant_id;
}

} // namespace pitch2
} // namespace jb
