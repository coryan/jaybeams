#include "jb/itch5/order_book.hpp"

jb::itch5::half_quote jb::itch5::order_book::best_bid() const {
  if (buy_.empty()) {
    return empty_bid();
  }
  auto i = buy_.begin();
  return half_quote(i->first, i->second);
}

jb::itch5::half_quote jb::itch5::order_book::worst_bid() const {
  if (buy_.empty()) {
    return empty_bid();
  }
  auto i = buy_.rbegin();
  return half_quote(i->first, i->second);
}

jb::itch5::half_quote jb::itch5::order_book::best_offer() const {
  if (sell_.empty()) {
    return empty_offer();
  }
  auto i = sell_.begin();
  return half_quote(i->first, i->second);
}

jb::itch5::half_quote jb::itch5::order_book::worst_offer() const {
  if (sell_.empty()) {
    return empty_offer();
  }
  auto i = sell_.rbegin();
  return half_quote(i->first, i->second);
}
