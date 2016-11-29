#include "jb/itch5/order_book_cache_aware.hpp"

// definition of tick_off_
jb::itch5::tick_t jb::itch5::order_book_cache_aware::tick_off_ = 5000;

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
  tick_t p_base_tick = p_base.as_integer();
  price4_t p_min, p_max;
  if (p_base_tick <= PX_DOLLAR_TICK) {
    // p_base < $1.00
    if (p_base_tick <= tick_off_) {
      p_min = price4_t(0);
      p_base_tick = tick_off_; // to use the full range
    } else {
      p_min = price4_t(p_base_tick - tick_off_);
    }
    tick_t rem_tick = tick_off_ + p_base_tick - PX_DOLLAR_TICK;
    p_max = (rem_tick > 0) ? price4_t(PX_DOLLAR_TICK + 100 * rem_tick)
                           : price4_t(p_base_tick + tick_off_);
    return price_range_t(p_min, p_max);
  }
  auto rem_tick = p_base_tick - 100 * tick_off_;
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

jb::itch5::tick_t jb::itch5::order_book_cache_aware::tick_offset() {
  return tick_off_;
}

void jb::itch5::order_book_cache_aware::tick_offset(tick_t const tick) {
  if (tick <= 0) {
    throw feed_error("Tick offset has to be greater than 0");
  }
  tick_off_ = tick;
}

bool jb::itch5::order_book_cache_aware::check_off_limits(
    buy_sell_indicator_t const side, price4_t const px) const {
  if (side == buy_sell_indicator_t('B')) {
    return (
        (std::get<0>(buy_price_range_) > px) or
        (std::get<1>(buy_price_range_) <= px));
  }
  return (
      (std::get<0>(sell_price_range_) < px) or
      (std::get<1>(sell_price_range_) >= px));
}

jb::itch5::level_t jb::itch5::order_book_cache_aware::price_levels(
    buy_sell_indicator_t const side, price4_t const pold,
    price4_t const pnew) const {
  if (side == buy_sell_indicator_t('B')) {
    if (pold > pnew) {
      auto tail_min = buy_.upper_bound(pold);
      auto tail_max = buy_.upper_bound(pnew);
      return std::distance(tail_min, tail_max);
    }
    auto tail_min = buy_.upper_bound(pnew);
    auto tail_max = buy_.upper_bound(pold);
    return std::distance(tail_min, tail_max);
  }
  if (pold < pnew) {
    auto tail_min = sell_.upper_bound(pold);
    auto tail_max = sell_.upper_bound(pnew);
    return std::distance(tail_min, tail_max);
  }
  auto tail_min = sell_.upper_bound(pnew);
  auto tail_max = sell_.upper_bound(pold);
  return std::distance(tail_min, tail_max);
}

jb::itch5::level_t jb::itch5::order_book_cache_aware::side_price_levels(
    buy_sell_indicator_t const side) {
  price4_t p_inside;
  price_range_t* ptr_range;
  if (side == buy_sell_indicator_t('B')) {
    p_inside = buy_.begin()->first;
    ptr_range = &buy_price_range_;
  } else {
    p_inside = sell_.begin()->first;
    ptr_range = &sell_price_range_;
  }
  if (check_off_limits(side, p_inside)) {
    auto old_p_min = std::get<0>(*ptr_range);
    // set the new price_range around the new p_inside
    *ptr_range = price_range(side, p_inside);
    auto new_p_min = std::get<0>(*ptr_range);
    // get the price levels between new and old p min
    return price_levels(side, old_p_min, new_p_min);
  }
  return 0;
}

jb::itch5::tick_t jb::itch5::order_book_cache_aware::num_ticks(
    price4_t const oldp, price4_t const newp) const {
  tick_t newp_tick = newp.as_integer();
  tick_t oldp_tick = oldp.as_integer();
  if ((newp_tick <= PX_DOLLAR_TICK) and (oldp_tick <= PX_DOLLAR_TICK)) {
    return std::abs(newp_tick - oldp_tick);
  }
  if ((newp_tick > PX_DOLLAR_TICK) and (oldp_tick > PX_DOLLAR_TICK)) {
    return std::abs(newp_tick - oldp_tick) / 100;
  }
  return (newp_tick > oldp_tick)
             ? PX_DOLLAR_TICK - oldp_tick + (newp_tick - PX_DOLLAR_TICK) / 100
             : (oldp_tick - PX_DOLLAR_TICK) / 100 + PX_DOLLAR_TICK - newp_tick;
}