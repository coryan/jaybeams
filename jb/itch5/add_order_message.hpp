#ifndef jb_itch5_add_order_message_hpp
#define jb_itch5_add_order_message_hpp

#include <jb/itch5/buy_sell_indicator.hpp>
#include <jb/itch5/message_header.hpp>
#include <jb/itch5/price_field.hpp>
#include <jb/itch5/stock_field.hpp>

namespace jb {
namespace itch5 {

/**
 * Represent an 'Add Order' message in the ITCH-5.0 protocol.
 */
struct add_order_message {
  constexpr static int message_type = u'A';

  message_header header;
  std::uint64_t order_reference_number;
  buy_sell_indicator_t buy_sell_indicator;
  int shares;
  stock_t stock;
  price4_t price;
};

/// Specialize decoder for a jb::itch5::add_order_message
template<bool V>
struct decoder<V,add_order_message> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static add_order_message r(
      std::size_t size, char const* buf, std::size_t off) {
    add_order_message x;
    x.header =
        decoder<V,message_header>       ::r(size, buf, off + 0);
    x.order_reference_number =
        decoder<V,std::uint64_t>        ::r(size, buf, off + 11);
    x.buy_sell_indicator =
        decoder<V,buy_sell_indicator_t> ::r(size, buf, off + 19);
    x.shares =
        decoder<V,std::uint32_t>        ::r(size, buf, off + 20);
    x.stock =
        decoder<V,stock_t>              ::r(size, buf, off + 24);
    x.price =
        decoder<V,price4_t>             ::r(size, buf, off + 32);
    return x;
  }
};

/// Streaming operator for jb::itch5::add_order_message.
std::ostream& operator<<(
    std::ostream& os, add_order_message const& x);

} // namespace itch5
} // namespace jb

#endif // jb_itch5_add_order_message_hpp
