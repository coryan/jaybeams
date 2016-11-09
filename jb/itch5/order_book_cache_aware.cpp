#include "jb/itch5/order_book_cache_aware.hpp"

jb::itch5::half_quote jb::itch5::order_book_cache_aware::best_bid() const {
  if (buy_.empty()) {
    return empty_bid();
  }
  auto i = buy_.begin();
  return half_quote(i->first, i->second);
}

jb::itch5::half_quote jb::itch5::order_book_cache_aware::best_offer() const {
  if (sell_.empty()) {
    return empty_offer();
  }
  auto i = sell_.begin();
  return half_quote(i->first, i->second);
}

jb::itch5::price_range_t jb::itch5::order_book_cache_aware::price_range(
    buy_sell_indicator_t side, price4_t p_base) {
  auto ret_range = price_range(p_base);
  if (side == buy_sell_indicator_t('B')) {
    return ret_range;
  }
  return price_range_t(std::get<1>(ret_range), std::get<0>(ret_range));    
}

jb::itch5::price_range_t jb::itch5::order_book_cache_aware::price_range(
    price4_t p_base) {
  int p_base_tick = p_base.as_integer();
  int base_tick = (p_base_tick > 10000) ? 100 : 1;
  if (p_base_tick <= (tick_off_ * base_tick)) {
    auto p_min = price4_t(0);
    auto p_max = price4_t(base_tick*(2*tick_off_));
    return price_range_t(p_min, p_max);
  }
  auto p_min = price4_t(p_base_tick - base_tick*tick_off_);
  auto p_max = price4_t(p_base_tick + base_tick*(tick_off_));
  return price_range_t(p_min, p_max);    
}

jb::itch5::price_range_t jb::itch5::order_book_cache_aware::price_range(
    buy_sell_indicator_t side) const {
  if (side == buy_sell_indicator_t('B')) {
    return buy_price_range_;
  }
  return sell_price_range_;
}

// definition of tick_off_
jb::itch5::tick_t jb::itch5::order_book_cache_aware::tick_off_ = 5000;

jb::itch5::tick_t jb::itch5::order_book_cache_aware::tick_offset() {
  return tick_off_;
}

void jb::itch5::order_book_cache_aware::tick_offset(
    tick_t tick) {
  if (tick <= 0) {
    throw feed_error("Tick offset has to be greater than 0");
  }
  tick_off_ = tick;
}






