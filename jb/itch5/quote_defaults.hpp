#ifndef jb_itch5_quote_defaults_hpp
#define jb_itch5_quote_defaults_hpp

#include <jb/itch5/price_field.hpp>

namespace jb {
namespace itch5 {

/// A simple representation for price + quantity
using half_quote = std::pair<price4_t, int>;

/// convenient value to represent an empty bid limit price
inline price4_t empty_bid_price() {
  return price4_t(0);
}

/// convenient value to represent an empty offer limit price
inline price4_t empty_offer_price() {
  return max_price_field_value<price4_t>();
}

/// The value used to represent an empty bid
inline half_quote empty_bid() {
  return half_quote(empty_bid_price(), 0);
}

/// The value used to represent an empty offer
inline half_quote empty_offer() {
  return half_quote(empty_offer_price(), 0);
}

} // namespace itch5
} // namespace jb

#endif // jb_itch5_quote_defaults_hpp
