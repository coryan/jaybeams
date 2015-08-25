#ifndef jb_itch5_stock_trading_action_message_hpp
#define jb_itch5_stock_trading_action_message_hpp

#include <jb/itch5/char_list_field.hpp>
#include <jb/itch5/message_header.hpp>
#include <jb/itch5/stock_field.hpp>

namespace jb {
namespace itch5 {

/**
 * Represent the 'Trading State' field on a 'Stock Trading Action' message.
 */
typedef char_list_field<u'H', u'P', u'Q', u'T'> trading_state_t;

/**
 * Represent the 'Reason' field in a 'Stock Trading Action' message.
 */
typedef short_string_field<4> reason_t;

/**
 * Represent a 'Stock Trading Action' message in the ITCH-5.0 protocol.
 */
struct stock_trading_action_message {
  constexpr static int message_type = u'H';

  message_header header;
  stock_t stock;
  trading_state_t trading_state;
  int reserved;
  reason_t reason;
};

/// Specialize decoder for a jb::itch5::stock_trading_action_message
template<bool V>
struct decoder<V,stock_trading_action_message> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static stock_trading_action_message r(
      std::size_t size, char const* buf, std::size_t off) {
    stock_trading_action_message x;
    x.header =
        decoder<V,message_header>  ::r(size, buf, off + 0);
    x.stock =
        decoder<V,stock_t>         ::r(size, buf, off + 11);
    x.trading_state =
        decoder<V,trading_state_t> ::r(size, buf, off + 19);
    x.reserved =
        decoder<V,std::uint8_t>    ::r(size, buf, off + 20);
    x.reason =
        decoder<V,reason_t>        ::r(size, buf, off + 21);
    return std::move(x);
  }
};

/// Streaming operator for jb::itch5::stock_trading_action_message.
std::ostream& operator<<(
    std::ostream& os, stock_trading_action_message const& x);

} // namespace itch5
} // namespace jb

#endif /* jb_itch5_stock_trading_action_message_hpp */
