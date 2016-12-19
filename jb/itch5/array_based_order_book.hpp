#ifndef jb_itch5_array_based_order_book_hpp
#define jb_itch5_array_based_order_book_hpp

#include <jb/itch5/price_field.hpp>
#include <jb/itch5/price_levels.hpp>
#include <jb/config_object.hpp>
#include <jb/feed_error.hpp>
#include <jb/log.hpp>

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <sstream>
#include <utility>
#include <vector>

namespace jb {
namespace itch5 {

/// A simple representation for price + quantity
using half_quote = std::pair<price4_t, int>;

/// The values used to represent the range of valid prices
/// LOWEST_PRICE < valid price < HIGHEST_PRICE
const price4_t LOWEST_PRICE = price4_t(0);
const price4_t HIGHEST_PRICE = max_price_field_value<price4_t>();
const int WIRE_MAX = HIGHEST_PRICE.as_integer();

/// price level limit between mill and penny
int constexpr TK_DOLLAR = 10000;

/**
 * Represent one side of the book. Class implementation of struct
 * array_based_order_book buy and side types.
 * @tparam compare_t function object class type to sort the side
 *
 * top_levels_ variables notation and definition:
 * - px_X : price4_t type. An actual price.
 *  - px_1 = price4_t(100000), price is $10.00
 *  - px_2 = price4_t(5000), price is c50 (fifty cents)
 * - tk_X : int type. A price level. First 10000 are mill, then penny.
 *  - tk_1 =  5010, price is c50.10
 *  - tk_2 = 10005, price is $1.05 ($1.00 + 5 penny)
 * - rel_X : size_t type. A relative position of a price at top_levels_
 *  - BUY side examples:
 *   - rel_1 = 20 (with px_begin_top_ = $30.00), tk_1 = 12920, px_1 = $30.20
 *   - rel_2 = 40 (with px_begin_top_ = c30.00), tk_2 = 3040, px_2 = c30.40
 *   - rel_3 = 1020 (with px_begin_top_ = c90.00), tk_3 = 10020, px_3 = $1.20
 *  - SELL side examples :
 *   - rel_1 = 20 (with px_begin_top_ = $30.00), tk_1 = 12880, px_1 = $29.80
 *   - rel_2 = 40 (with px_begin_top_ = c30.00), tk_2 = 2960, px_2 = c29.60
 *   - rel_3 = 1020 (with px_begin_top_ = $1.10), tk_3 = 8990, px_3 = c89.90
 */
template <typename compare_t>
class array_based_book_side {
public:
  explicit array_based_book_side(std::size_t sz)
      : better_()
      , max_size_(sz)
      , top_levels_(sz, 0)
      , bottom_levels_()
      , px_inside_(empty_quote().first.as_integer())
      , px_begin_top_(empty_quote().first.as_integer())
      , px_end_top_(empty_quote().first.as_integer()) {
  }

  /// The value used to represent an empty bid
  static half_quote empty_bid() {
    return half_quote(LOWEST_PRICE, 0);
  }

  /// The value used to represent an empty offer
  static half_quote empty_offer() {
    return half_quote(HIGHEST_PRICE, 0);
  }

  /// @returns an empty bid or offer based on compare function
  /// empty bid for less, empty offer for greater.
  half_quote empty_quote() const {
    if (better_(price4_t(1), price4_t(0))) {
      // buy side
      return empty_bid();
    }
    // sell side
    return empty_offer();
  }

  /// @returns the best bid price and quantity.
  /// The inside is always at the top_levels_ (empty_quote.price when empty)
  half_quote best_quote() const {
    if (px_inside_ == empty_quote().first) {
      return empty_quote();
    }
    int rel_px = price_to_relative(px_inside_);
    return half_quote(px_inside_, top_levels_.at(rel_px));
  }

  /// @returns the worst bid price and quantity.
  /// Check if the side is empty first (return empty_quote in this case)
  /// The worst price is at the bottom_levels (if not empty),
  /// at the top_levels otherwise.
  half_quote worst_quote() const {
    if (px_inside_ == empty_quote().first) {
      return empty_quote(); // empty side
    }
    if (!bottom_levels_.empty()) {
      // ... worst price is at the bottom_levels_
      auto i = bottom_levels_.rbegin(); // get the worst bottom_levels price
      return half_quote(i->first, i->second);
    }
    // ... worst price at the top_levels_
    int rel_worst = relative_worst_top_level();
    price4_t px_worst = relative_to_price(rel_worst);
    return half_quote(px_worst, top_levels_.at(rel_worst));
  }

  /// @returns the number of levels with non-zero quantity for the order side.
  /// This is the size of bottom_levels_ plus all non zero top_levels values
  std::size_t count() const {
    if (!bottom_levels_.empty()) {
      return bottom_levels_.size() + top_levels_count();
    }
    return top_levels_count();
  }

  /**
   * Add a price and quantity to the side order book.
   *
   * @param px the price of the new order
   * @param qty the quantity of the new order
   * @returns true if the inside changed
   *
   * Validates:
   * - if px is off limits (<= empty_bid price or >= empty_offer price)
   * - if qty <= 0
   * - throws an exception if this is the case
   * .
   * Then:
   * - if px is worse than the px_begin_top_ then px goes to bottom_levels
   * - if px is better than the inside then changes inside, redefines limits
   * if needed (moving the tail to bottom_levels if any)
   * - finally, updates qty at the relative(px)
   */
  bool add_order(price4_t px, int qty) {
    // verify that px and qty are valid values. Throws feed_error otherwise
    if ((px <= LOWEST_PRICE) or (px >= HIGHEST_PRICE) or (qty <= 0)) {
      std::ostringstream os;
      os << "array_based_order_book::add_order value(s) out of range."
         << " px=" << px << " qty=" << qty;
      throw jb::feed_error(os.str());
    }
    // check if px is worse than the first price of the top_levels_
    if (better_(px_begin_top_, px)) {
      // emplace the price at the bottom_levels, and return (false)
      auto emp_tup = bottom_levels_.emplace(px, 0);
      emp_tup.first->second += qty;
      return false;
    }
    // check if px is equal or better than the current inside
    if (not better_(px_inside_, px)) {
      // ... check if limit redefine is needed
      if (not better_(px_end_top_, px)) {
        // get the new limits based on the new inside px
        auto limits = get_limit_top_prices(px);
        // move the tail [px_begin_top_, new px_begin_top_) to bottom_levels_
        move_top_to_bottom(std::get<0>(limits));
        // ... redefine the limits
        px_begin_top_ = std::get<0>(limits);
        px_end_top_ = std::get<1>(limits);
      }
      // if the px inside changed, updated
      if (px_inside_ != px) {
        px_inside_ = px;
      }
      // update top_levels_
      auto rel_px = price_to_relative(px_inside_);
      top_levels_.at(rel_px) += qty;
      return true; // the inside changed
    }
    // is a top_levels change different than the inside
    // udates top_levels_ with new qty
    auto rel_px = price_to_relative(px);
    top_levels_.at(rel_px) += qty;
    return false;
  }

  /**
   * Reduce the quantity for a given price.
   *
   * @param px the price of the order that was reduced
   * @param qty the quantity reduced in the order
   * @returns true if the inside changed
   *
   * Validates a positive qty
   * Checks and handles if px is a bottom_level_ price
   * Checks if top_levels_ is empty after reducing
   * If so, limits have to be redefined, and tail moved into top_levels
   */
  bool reduce_order(price4_t px, int qty) {
    // validates input values, qty has to be a positive number
    if (qty <= 0) {
      std::ostringstream os;
      os << "array_based_order_book::reduce_order value(s) out of range."
         << " px=" << px << " qty=" << qty;
      throw jb::feed_error(os.str());
    }

    // check and handles if it is a bottom_level_ price
    if (better_(px_begin_top_, px)) {
      if (bottom_levels_.empty()) {
        std::ostringstream os;
        os << "array_based_order_book::reduce_order."
           << " Trying to reduce a non-existing bottom_levels_ price"
           << " (empty bottom_levels_)."
           << " px_begin_top_=" << px_begin_top_ << " px=" << px
           << " qty=" << qty;
        throw jb::feed_error(os.str());
      }
      auto price_it = bottom_levels_.find(px);
      if (price_it == bottom_levels_.end()) {
        std::ostringstream os;
        os << "array_based_order_book::reduce_order."
           << " Trying to reduce a non-existing bottom_levels_ price."
           << " px_begin_top_=" << px_begin_top_ << " px=" << px
           << " qty=" << qty;
        throw jb::feed_error(os.str());
      }
      // ... reduce the quantity ...
      price_it->second -= qty;
      if (price_it->second < 0) {
        // ... this is "Not Good[tm]", somehow we missed an order or
        // processed a delete twice ...
        JB_LOG(warning) << "negative quantity in order book";
      }
      // now we can erase this element if updated qty <=0
      if (price_it->second <= 0) {
        bottom_levels_.erase(price_it);
      }
      return false;
    }

    // handles the top_levels_ price
    if (better_(px, px_inside_)) {
      std::ostringstream os;
      os << "array_based_order_book::reduce_order."
         << " Trying to reduce a non-existing top_levels_ price"
         << " (better px_inside)."
         << " px_begin_top_=" << px_begin_top_ << " px_inside_=" << px_inside_
         << " px_end_top_=" << px_end_top_ << " px=" << px << " qty=" << qty;
      throw jb::feed_error(os.str());
    }
    // get px relative position
    auto rel_px = price_to_relative(px);
    if (top_levels_.at(rel_px) == 0) {
      std::ostringstream os;
      os << "array_based_order_book::reduce_order."
         << " Trying to reduce a non-existing top_levels_ price"
         << " (top_levels_[rel_px] == 0)."
         << " px_begin_top_=" << px_begin_top_ << " px_inside_=" << px_inside_
         << " px_end_top_=" << px_end_top_ << " px relative position=" << rel_px
         << " px=" << px << " qty=" << qty;
      throw jb::feed_error(os.str());
    }

    // ... reduce the quantity ...
    top_levels_.at(rel_px) -= qty;
    if (top_levels_.at(rel_px) < 0) {
      // ... this is "Not Good[tm]", somehow we missed an order or
      // processed a delete twice ...
      JB_LOG(warning) << "negative quantity in order book";
      top_levels_.at(rel_px) = 0; // cant't be negative
    }
    // ... if it is not the inside we are done
    if (px != px_inside_) {
      return false;
    }
    // Is inside, ... now check if it was removed
    if (top_levels_.at(rel_px) == 0) {
      // gets the new inside (if any)
      px_inside_ = next_best_price();
      if (px_inside_ == empty_quote().first) {
        // last top_levels_ price was removed...
        // ... get the new inside from the bottom_levels
        if (!bottom_levels_.empty()) {
          auto new_inside_it = bottom_levels_.begin();
          px_inside_ = new_inside_it->first;
        }
        // redefine limits
        auto limits = get_limit_top_prices(px_inside_);
        px_begin_top_ = std::get<0>(limits);
        px_end_top_ = std::get<1>(limits);
        // move tail prices from bottom_levels (px_begin_top_ {, .begin()})
        move_bottom_to_top();
      }
    }
    return true; // it is an inside change
  }

  /**
   * Testing hook.
   * @returns true is px1 < px2
   * To test different implementations for buy and sell sides
   */
  bool check_better(price4_t const& px1, price4_t const& px2) const {
    return better_(px1, px2);
  }

  /** Testing hook.
   * @returns {px_begin_top_, px_end_top_}
   */
  auto get_limits() const {
    return std::make_tuple(px_begin_top_, px_end_top_);
  }

  /** Testing hook.
   * @returns {bottom_levels_ size , top_levels_ size}
   */
  auto get_counts() const {
    return std::make_pair(bottom_levels_.size(), top_levels_count());
  }

  /** Testing hooks to increase coverage
   */
  auto test_price_to_relative(price4_t const& px) const {
    return price_to_relative(px);
  }
  // to test when is an empty side
  auto test_relative_worst_top_level() const {
    return relative_worst_top_level();
  }
  // to test with a px worse than px_begin_top
  void test_move_top_to_bottom(price4_t const& px) {
    move_top_to_bottom(px);
  }

private:
  /**
   * Transforms a top_levels_ relative position into an absolute price.
   * @param rel relative position of a price at top_levels_
   * @returns the absolute price
   */
  price4_t relative_to_price(int rel) const {
    int rel_tick = (better_(price4_t(1), price4_t(0))) ? rel : max_size_ - rel;
    price4_t px_base =
        (better_(price4_t(1), price4_t(0))) ? px_begin_top_ : px_end_top_;
    return relative_above_base(px_base, rel_tick);
  }

  /**
   * @param px_base base absolute price
   * @param rel_tick price levels above px_base
   * @returns absolute price rel_tick price levels above px_base
   */
  price4_t
  relative_above_base(price4_t const& px_base, int const rel_tick) const {
    // check if px_base is above $1.00
    if (px_base > price4_t(TK_DOLLAR)) {
      // already better than $1.00, then rel_tick is in penny
      int wire_new = px_base.as_integer() + 100 * rel_tick;
      wire_new = (wire_new < WIRE_MAX) ? wire_new : WIRE_MAX;
      return price4_t(wire_new);
    }
    // px_base is below $1.00...
    // ... get the ticks to reach $1.00
    int tk_mill = price_levels(px_base, price4_t(TK_DOLLAR));
    // ... verify if rel_tick is not enough to reach it
    if (rel_tick < tk_mill) {
      // rel_tick is in mills
      return px_base + price4_t(rel_tick);
    }
    // get the ticks above $1.00
    int tk_penny = rel_tick - tk_mill;
    return price4_t(TK_DOLLAR + tk_penny * 100);
  }

  /**
   * @param px_base absolute base price
   * @param rel_tick price levels below px_base
   * @returns an absolute price that is rel_tick price levels below px_base
   */
  price4_t
  relative_below_base(price4_t const& px_base, int const rel_tick) const {
    // check if px_base is above $1.00
    if (px_base < price4_t(TK_DOLLAR)) {
      // already below than $1.00, then rel_tick is in mill
      int rel_base = px_base.as_integer();
      rel_base = (rel_base < rel_tick) ? 0 : rel_base - rel_tick;
      return price4_t(rel_base);
    }
    // px_base is above $1.00...
    // ... get the ticks to reach $1.00
    int tk_penny = price_levels(price4_t(TK_DOLLAR), px_base);
    // ... verify if rel_tick is not enough to reach it
    if (rel_tick < tk_penny) {
      // rel_tick is in penny
      int rel_base = px_base.as_integer() - rel_tick * 100;
      return price4_t(rel_base);
    }
    // get the ticks below $1.00
    int tk_mill = rel_tick - tk_penny;
    int rel_base = px_base.as_integer();
    rel_base = (tk_mill > TK_DOLLAR) ? 0 : TK_DOLLAR - tk_mill;
    return price4_t(rel_base);
  }

  /**
   * Transforms an absolute price into a top_levels_ relative position.
   * px has to be better or equal to px_begin_top_
   * @param px an absolute price
   * @returns top_levels_ relative position of px compare with px_begin_top_
   */
  std::size_t price_to_relative(price4_t const& px) const {
    if (better_(px_begin_top_, px)) {
      std::ostringstream os;
      os << "array_based_order_book::price_to_relative."
         << " Price out of range"
         << " px_begin_top_=" << px_begin_top_ << " px=" << px;
      throw jb::feed_error(os.str());
    }
    int tk_px = price_levels(price4_t(0), px);
    int tk_begin_top = price_levels(price4_t(0), px_begin_top_);
    return std::abs(tk_px - tk_begin_top);
  }

  /**
   * @returns relative position of the worst valid price at top_levels
   */
  std::size_t relative_worst_top_level() const {
    for (std::size_t i = 0; i != max_size_; ++i) {
      if (top_levels_.at(i) != 0) {
        return i; // found it
      }
    }
    // wow! this is a problem... top_levels_ was not considered empty
    std::ostringstream os;
    os << "array_based_order_book::relative_worst_top_level."
       << " Seems to be an empty side"
       << " px_inside=" << px_inside_;
    throw jb::feed_error(os.str());
  }

  /// @returns number of valid prices (>0) at top_levels_
  std::size_t top_levels_count() const {
    // counts from 0 .. relative position of px_inside_
    if (px_inside_ == empty_quote().first) {
      return 0; // empty side
    }
    int rel_px = price_to_relative(px_inside_);
    int res = 0;
    for (int i = 0; i != rel_px + 1; ++i) {
      if (top_levels_.at(i) != 0) {
        res++;
      }
    }
    return res;
  }

  /// @returns pair of absolute prices that are max_size/2 worse
  /// and better than px, limited to valid prices
  /// if px is at the limit, return and empty quote values
  auto get_limit_top_prices(price4_t const& px) const {
    if (px == empty_quote().first) {
      return std::make_pair(empty_quote().first, empty_quote().first);
    }
    auto px_low = relative_below_base(px, max_size_ / 2);
    auto px_low_base = relative_above_base(px_low, max_size_ / 2);
    auto px_high = relative_above_base(px_low_base, max_size_ / 2);
    auto px_high_base = relative_below_base(px_high, max_size_ / 2);
    if (px_low_base != px_high_base) {
      px_low = relative_below_base(px_high_base, max_size_ / 2);
    }
    return (better_(price4_t(1), price4_t(0)))
               ? std::make_pair(px_low, px_high)
               : std::make_pair(px_high, px_low);
  }

  /**
   * Move prices from px_begin_top_ to px_max (excluded)
   * out of top_levels_ to bottom_levels_.
   *
   * @param px_max first limit price that is not moved out.
   *
   * Validates that px_max is in range (better or equal to px_begin_top_)
   * Inserts the prices into bottom_levels_
   * Shift top_levels_ prices in order for rel_max to become relative 0
   * Clear (value = 0) former relative position of moved prices
   */
  void move_top_to_bottom(price4_t const px_max) {
    if (better_(px_begin_top_, px_max)) {
      // wow! this is a problem... incorrect value
      std::ostringstream os;
      os << "array_based_order_book::move_top_to_bottom."
         << " Value out of range"
         << " px_begin_top_=" << px_begin_top_ << " px_max=" << px_max;
      throw jb::feed_error(os.str());
    }
    // if px_max is better than px_inside_
    if (better_(px_max, px_inside_)) {
      // ... all top_prices_ have to be moved out
      // valid prices are from 0 .. relative (px_inside_) included
      auto rel_inside = price_to_relative(px_inside_);
      for (std::size_t i = 0; i != rel_inside + 1; ++i) {
        // move out and clear the price (if valid)
        if (top_levels_.at(i) != 0) {
          price4_t px_i = relative_to_price(i);
          bottom_levels_.emplace(px_i, top_levels_.at(i));
          top_levels_.at(i) = 0;
        }
      }
      return;
    }
    // px_max is worse than px_inside
    // prices to move out are from 0.. relative (px_max) excluded
    auto rel_px_max = price_to_relative(px_max);
    for (std::size_t i = 0; i != rel_px_max; ++i) {
      // ... move out the price i (if valid)
      if (top_levels_.at(i) != 0) {
        price4_t px_i = relative_to_price(i);
        bottom_levels_.emplace(px_i, top_levels_.at(i));
        top_levels_.at(i) = 0;
      }
    }
    // shift price down rel_px_max positions
    // from px_max to px_inside both included...
    // ...(the are no better prices than px_inside_)
    auto rel_px_inside = price_to_relative(px_inside_);
    std::size_t j = 0;
    for (std::size_t i = rel_px_max; i <= rel_px_inside; ++i, ++j) {
      if (top_levels_.at(i) != 0) {
        top_levels_.at(j) = top_levels_.at(i);
        top_levels_.at(i) = 0;
      }
    }
  }

  /**
   * Move relative prices from best price to px_begin_top* (included)
   * out of bottom_levels_ to top_levels_.
   * *px_begin_top_ is updated before calling this function.
   */
  void move_bottom_to_top() {
    if (bottom_levels_.empty()) {
      return; // nothing to move in
    }
    // traverse all bottom_levels_ prices worse than or equal to px_begin_top_
    auto le = bottom_levels_.begin();
    for (/* */; le != bottom_levels_.end(); ++le) {
      if (not better_(px_begin_top_, le->first)) {
        // move price to top_levels_
        auto rel_px = price_to_relative(le->first);
        top_levels_.at(rel_px) = le->second;
      } else {
        // no more good prices to move...
        break;
      }
    }
    // erase the moved prices..
    bottom_levels_.erase(bottom_levels_.begin(), le);
  }

  /**
   * @returns the best next price from the inside.
   * (return empty quote price if none available).
   */
  price4_t next_best_price() const {
    // begins checking below the current inside
    auto rel_inside = price_to_relative(px_inside_);
    for (int i = rel_inside - 1; i != -1; --i) {
      if (top_levels_.at(i) != 0) {
        // got the next one...
        return relative_to_price(i);
      }
    }
    return empty_quote().first;
  }

private:
  /// function object for inequality comparison of template parameter type.
  /// if better_(p1,p2) is true means p1 is strictly a better price than p2
  compare_t better_;

  /// top_levels_ max size
  std::size_t max_size_;

  /// the best relative prices and quantity
  std::vector<int> top_levels_;

  /// the worst (tail) absolute prices and quantity
  std::map<price4_t, int, compare_t> bottom_levels_;

  /// better absolute price of top_levels_ range
  price4_t px_inside_;

  /// one absolute price past-the-worst price in top_levels_ range
  price4_t px_begin_top_;

  /// absolute price at the inside
  price4_t px_end_top_;
};

/**
 * Define the types of buy and sell side classes.
 *
 * It is used as template parameter book_type of the
 * template classes order_book and compute_book:
 * - usage: jb::itch5::order_book<jb::itch5::array_based_order_book>
 */
struct array_based_order_book {
  using buys_t = array_based_book_side<std::greater<price4_t>>;
  using sells_t = array_based_book_side<std::less<price4_t>>;
};

} // namespace itch5
} // namespace jb

#endif // jb_itch5_array_based_order_book_hpp