#ifndef jb_itch5_net_order_imbalance_indicator_message_hpp
#define jb_itch5_net_order_imbalance_indicator_message_hpp

#include <jb/itch5/cross_type.hpp>
#include <jb/itch5/message_header.hpp>
#include <jb/itch5/price_field.hpp>
#include <jb/itch5/stock_field.hpp>

namespace jb {
namespace itch5 {

/**
 * Represent the 'Imbalance Direction' field in the 'Net order
 * Imbalance Indicator' message.
 */
typedef char_list_field<u'B', u'S', u'N', u'O'> imbalance_direction_t;

/**
 * Represent the 'Price Variation Indicator' field in the 'Net Order
 * Imbalance Indicator' message.
 */
typedef char_list_field<
  u'L', u'1', u'2', u'3', u'4', u'5', u'6', u'7', u'8', u'9', u'A', u'B',
  u'C', u' '>  price_variation_indicator_t;

/**
 * Represent an 'Net Order Imbalance Indicator' message in the
 * ITCH-5.0 protocol.
 */
struct net_order_imbalance_indicator_message {
  constexpr static int message_type = u'I';

  message_header header;
  std::uint64_t paired_shares;
  std::uint64_t imbalance_shares;
  imbalance_direction_t imbalance_direction;
  stock_t stock;
  price4_t far_price;
  price4_t near_price;
  price4_t current_reference_price;
  cross_type_t cross_type;
  price_variation_indicator_t price_variation_indicator;
};

/// Specialize decoder for a jb::itch5::net_order_imbalance_indicator_message
template<bool V>
struct decoder<V,net_order_imbalance_indicator_message> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static net_order_imbalance_indicator_message r(
      std::size_t size, char const* buf, std::size_t off) {
    net_order_imbalance_indicator_message x;
    x.header =
        decoder<V,message_header>              ::r(size, buf, off + 0);
    x.paired_shares =
        decoder<V,std::uint64_t>               ::r(size, buf, off + 11);
    x.imbalance_shares =
        decoder<V,std::uint64_t>               ::r(size, buf, off + 19);
    x.imbalance_direction =
        decoder<V,imbalance_direction_t>       ::r(size, buf, off + 27);
    x.stock =
        decoder<V,stock_t>                     ::r(size, buf, off + 28);
    x.far_price =
        decoder<V,price4_t>                    ::r(size, buf, off + 36);
    x.near_price =
        decoder<V,price4_t>                    ::r(size, buf, off + 40);
    x.current_reference_price =
        decoder<V,price4_t>                    ::r(size, buf, off + 44);
    x.cross_type =
        decoder<V,cross_type_t>                ::r(size, buf, off + 48);
    x.price_variation_indicator =
        decoder<V,price_variation_indicator_t> ::r(size, buf, off + 49);
    return x;
  }
};

/// Streaming operator for jb::itch5::net_order_imbalance_indicator_message.
std::ostream& operator<<(
    std::ostream& os, net_order_imbalance_indicator_message const& x);

} // namespace itch5
} // namespace jb

#endif // jb_itch5_net_order_imbalance_indicator_message_hpp
