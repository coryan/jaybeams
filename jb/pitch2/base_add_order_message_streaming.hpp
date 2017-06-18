#ifndef jb_pitch2_base_add_order_message_streaming_hpp
#define jb_pitch2_base_add_order_message_streaming_hpp

#include <jb/pitch2/base_add_order_message.hpp>

#include <ostream>

namespace jb {
namespace pitch2 {

/// Streaming operator for jb::pitch2::add_order_message.
template <typename qT, typename sT, typename pT>
std::ostream&
operator<<(std::ostream& os, base_add_order_message<qT, sT, pT> const& x) {
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

#endif // jb_pitch2_base_add_order_message_streaming_hpp
