#ifndef jb_itch5_quote_defaults_hpp
#define jb_itch5_quote_defaults_hpp

#include <jb/itch5/price_field.hpp>

namespace jb {
namespace itch5 {

/// A simple representation for price + quantity
using half_quote = std::pair<price4_t, int>;

/// The value used to represent an empty bid
static half_quote empty_bid() {
  return half_quote(price4_t(0), 0);
}

/// The value used to represent an empty offer
static half_quote empty_offer() {
  return half_quote(max_price_field_value<price4_t>(), 0);
}

} // namespace itch5
} // namespace jb

#endif // jb_itch5_quote_defaults_hpp
