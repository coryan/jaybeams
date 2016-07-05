#ifndef jb_itch5_cross_trade_message_hpp
#define jb_itch5_cross_trade_message_hpp

#include <jb/itch5/cross_type.hpp>
#include <jb/itch5/message_header.hpp>
#include <jb/itch5/price_field.hpp>
#include <jb/itch5/stock_field.hpp>

namespace jb {
namespace itch5 {

/**
 * Represent an 'Add Order' message in the ITCH-5.0 protocol.
 */
struct cross_trade_message {
  constexpr static int message_type = u'Q';

  message_header header;
  std::uint64_t shares;
  stock_t stock;
  price4_t cross_price;
  std::uint64_t match_number;
  cross_type_t cross_type;
};

/// Specialize decoder for a jb::itch5::cross_trade_message
template<bool V>
struct decoder<V,cross_trade_message> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static cross_trade_message r(
      std::size_t size, void const* buf, std::size_t off) {
    cross_trade_message x;
    x.header =
        decoder<V,message_header>       ::r(size, buf, off + 0);
    x.shares =
        decoder<V,std::uint64_t>        ::r(size, buf, off + 11);
    x.stock =
        decoder<V,stock_t>              ::r(size, buf, off + 19);
    x.cross_price =
        decoder<V,price4_t>             ::r(size, buf, off + 27);
    x.match_number =
        decoder<V,std::uint64_t>        ::r(size, buf, off + 31);
    x.cross_type =
        decoder<V,cross_type_t>         ::r(size, buf, off + 39);
    return x;
  }
};

/// Streaming operator for jb::itch5::cross_trade_message.
std::ostream& operator<<(
    std::ostream& os, cross_trade_message const& x);

} // namespace itch5
} // namespace jb

#endif // jb_itch5_cross_trade_message_hpp
