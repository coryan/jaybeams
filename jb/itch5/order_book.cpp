#include <jb/itch5/order_book.hpp>

jb::itch5::half_quote jb::itch5::order_book::best_bid() const {
  if (buy_.empty()) {
    return half_quote(price4_t(0), 0);
  }
  auto i = buy_.begin();
  return half_quote(i->first, i->second);
}

jb::itch5::half_quote jb::itch5::order_book::best_offer() const {
  if (sell_.empty()) {
    return half_quote(max_price_field_value<price4_t>(), 0);
  }
  auto i = sell_.begin();
  return half_quote(i->first, i->second);
}
