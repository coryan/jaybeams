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

jb::itch5::price_range_t
jb::itch5::order_book_cache_aware::price_range(price4_t p_base) {
  int p_base_tick = p_base.as_integer();
  price4_t p_min, p_max;
  int rem_tick;
  if (p_base_tick <= PX_DOLLAR_TICK) {
    // p_base < $1.00
    if (p_base_tick <= tick_off_) {
      p_min = price4_t(0);
      p_base_tick = tick_off_; // to use the full range
    } else {
      p_min = price4_t(p_base_tick - tick_off_);
    }
    rem_tick = tick_off_ + p_base_tick - PX_DOLLAR_TICK;
    p_max = (rem_tick > 0) ? price4_t(PX_DOLLAR_TICK + 100 * rem_tick)
                           : price4_t(p_base_tick + tick_off_);
    return price_range_t(p_min, p_max);
  }
  rem_tick = p_base_tick - 100 * tick_off_;
  if (rem_tick > PX_DOLLAR_TICK) {
    p_min = price4_t(rem_tick);
  } else {
    if (rem_tick <= 0) {
      p_min = price4_t(0);
    } else {
      p_min = price4_t(rem_tick);
    }
  }
  p_max = price4_t(p_base_tick + 100 * tick_off_);
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

void jb::itch5::order_book_cache_aware::tick_offset(tick_t tick) {
  if (tick <= 0) {
    throw feed_error("Tick offset has to be greater than 0");
  }
  tick_off_ = tick;
}
