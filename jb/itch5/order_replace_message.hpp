#ifndef jb_itch5_order_replace_message_hpp
#define jb_itch5_order_replace_message_hpp

#include <jb/itch5/buy_sell_indicator.hpp>
#include <jb/itch5/message_header.hpp>
#include <jb/itch5/price_field.hpp>
#include <jb/itch5/stock_field.hpp>

namespace jb {
namespace itch5 {

/**
 * Represent an 'Order Replace' message in the ITCH-5.0 protocol.
 */
struct order_replace_message {
  constexpr static int message_type = u'U';

  message_header header;
  std::uint64_t original_order_reference_number;
  std::uint64_t new_order_reference_number;
  int shares;
  price4_t price;
};

/// Specialize decoder for a jb::itch5::order_replace_message
template <bool V> struct decoder<V, order_replace_message> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static order_replace_message r(std::size_t size, void const* buf,
                                 std::size_t off) {
    order_replace_message x;
    x.header = decoder<V, message_header>::r(size, buf, off + 0);
    x.original_order_reference_number =
        decoder<V, std::uint64_t>::r(size, buf, off + 11);
    x.new_order_reference_number =
        decoder<V, std::uint64_t>::r(size, buf, off + 19);
    x.shares = decoder<V, std::uint32_t>::r(size, buf, off + 27);
    x.price = decoder<V, price4_t>::r(size, buf, off + 31);
    return x;
  }
};

/// Streaming operator for jb::itch5::order_replace_message.
std::ostream& operator<<(std::ostream& os, order_replace_message const& x);

} // namespace itch5
} // namespace jb

#endif // jb_itch5_order_replace_message_hpp
