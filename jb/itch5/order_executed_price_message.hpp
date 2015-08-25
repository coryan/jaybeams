#ifndef jb_itch5_order_executed_price_message_hpp
#define jb_itch5_order_executed_price_message_hpp

#include <jb/itch5/char_list_field.hpp>
#include <jb/itch5/order_executed_message.hpp>
#include <jb/itch5/price_field.hpp>

namespace jb {
namespace itch5 {

/**
 * @typedef printable_t
 *
 * Represent the 'Printable' field on a 'Order Executed with Price'
 * message.
 */
typedef char_list_field<u'Y', u'N'> printable_t;

/**
 * Represent an 'Order Executed with Price' message in the ITCH-5.0 protocol.
 */
struct order_executed_price_message : public order_executed_message {
  constexpr static int message_type = u'C';

  printable_t printable;
  price4_t execution_price;

  order_executed_price_message& operator=(order_executed_message const& rhs) {
    static_cast<order_executed_message*>(this)->operator=(rhs);
    return *this;
  }
  order_executed_price_message& operator=(order_executed_message&& rhs) {
    static_cast<order_executed_message*>(this)->operator=(rhs);
    return *this;
  }
};

/// Specialize decoder for a jb::itch5::order_executed_price_message
template<bool V>
struct decoder<V,order_executed_price_message> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static order_executed_price_message r(
      std::size_t size, char const* buf, std::size_t off) {
    order_executed_price_message x;
    x =
        decoder<V,order_executed_message> ::r(size, buf, off + 0);
    x.printable =
        decoder<V,printable_t>            ::r(size, buf, off + 31);
    x.execution_price =
        decoder<V,price4_t>               ::r(size, buf, off + 32);
    return std::move(x);
  }
};

/// Streaming operator for jb::itch5::order_executed_price_message.
std::ostream& operator<<(
    std::ostream& os, order_executed_price_message const& x);

} // namespace itch5
} // namespace jb

#endif /* jb_itch5_order_executed_price_message_hpp */
