#ifndef jb_itch5_ipo_quoting_period_update_message_hpp
#define jb_itch5_ipo_quoting_period_update_message_hpp

#include <jb/itch5/char_list_field.hpp>
#include <jb/itch5/message_header.hpp>
#include <jb/itch5/price_field.hpp>
#include <jb/itch5/seconds_field.hpp>
#include <jb/itch5/stock_field.hpp>

namespace jb {
namespace itch5 {

/**
 * Represent the 'IPO Quotation Release Qualifier' field on a 'IPO
 * Quoting Period Update' message.
 */
typedef char_list_field<u'A', u'C'> ipo_quotation_release_qualifier_t;

/**
 * Represent a 'IPO Quotation Release Update' message in the ITCH-5.0
 * protocol.
 */
struct ipo_quoting_period_update_message {
  constexpr static int message_type = u'K';

  message_header header;
  stock_t stock;
  seconds_field ipo_quotation_release_time;
  ipo_quotation_release_qualifier_t ipo_quotation_release_qualifier;
  price4_t ipo_price;
};

/// Specialize decoder for a jb::itch5::ipo_quoting_period_update_message
template<bool V>
struct decoder<V,ipo_quoting_period_update_message> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static ipo_quoting_period_update_message r(
      std::size_t size, char const* buf, std::size_t off) {
    ipo_quoting_period_update_message x;
    x.header =
        decoder<V,message_header>                    ::r(size, buf, off + 0);
    x.stock =
        decoder<V,stock_t>                           ::r(size, buf, off + 11);
    x.ipo_quotation_release_time =
        decoder<V,seconds_field>                     ::r(size, buf, off + 19);
    x.ipo_quotation_release_qualifier =
        decoder<V,ipo_quotation_release_qualifier_t> ::r(size, buf, off + 23);
    x.ipo_price =
        decoder<V,price4_t>                          ::r(size, buf, off + 24);
    return x;
  }
};

/// Streaming operator for jb::itch5::ipo_quoting_period_update_message.
std::ostream& operator<<(
    std::ostream& os, ipo_quoting_period_update_message const& x);

} // namespace itch5
} // namespace jb

#endif /* jb_itch5_ipo_quoting_period_update_message_hpp */
