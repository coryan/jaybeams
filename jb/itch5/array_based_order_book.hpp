#ifndef jb_itch5_array_based_order_book_hpp
#define jb_itch5_array_based_order_book_hpp

#include <jb/itch5/price_field.hpp>
#include <jb/itch5/price_levels.hpp>
#include <jb/itch5/quote_defaults.hpp>
#include <jb/assert_throw.hpp>
#include <jb/config_object.hpp>
#include <jb/feed_error.hpp>
#include <jb/log.hpp>

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <utility>
#include <vector>

namespace jb {
namespace itch5 {

/**
 * Refactor add_order() validation to a non-template standalone function.
 */
void validate_add_order_params(int, price4_t px = price4_t(0));
void raise_exception(std::string, std::size_t, price4_t, int);
void raise_exception(
    std::string, std::size_t, std::size_t, std::size_t, price4_t, int);

template <typename compare_t>
class array_based_book_side;

/**
 * Define the types of buy and sell side classes.
 *
 * It is used as template parameter book_type of the
 * template classes order_book and compute_book:
 * - usage: jb::itch5::order_book<jb::itch5::array_based_order_book>
 */
struct array_based_order_book {
  using buys_t = array_based_book_side<std::greater<std::size_t>>;
  using sells_t = array_based_book_side<std::less<std::size_t>>;
  class config;
};

/**
 * Configure an array_based_order_book config object
 */
class array_based_order_book::config : public jb::config_object {
public:
  config();
  config_object_constructors(config);

  /// Validate the configuration
  void validate() const override;

  jb::config_attribute<config, int> max_size;
};

/**
 * Represent one side of the book.
 *
 * This implementation uses a top_levels_ fix sized vector<price4_t> to
 * keep the prices around the inside. We are hoping this makes a faster
 * order book side (instead of map based).
 *
 * We have observed that most of the changes to an order book happen
 * in the levels closer to the inside.  We are trying to exploit this
 * observation by using a vector<> for the N levels closer to the inside,
 * as updates in a vector should be O(1) instead of O(log N) for tree-based
 * implementations, and O(1) (with a larger constant) for hash-based
 * implementations.
 * This introduces some complexity: maintaining all the levels in the vector
 * would consume too much memory, so we use a vector for the N levels
 * closer to the inside, and a map for all the other levels.
 *
 * @tparam compare_t function object class type to sort the side
 *
 */
template <typename compare_t>
class array_based_book_side {
public:
  explicit array_based_book_side(array_based_order_book::config const& cfg)
      : max_size_(cfg.max_size())
      , top_levels_(cfg.max_size(), 0)
      , bottom_levels_()
      , tk_inside_(
            price_levels(price4_t(0), side<compare_t>::empty_quote_price()))
      , tk_begin_top_(tk_inside_)
      , tk_end_top_(tk_inside_) {
  }

  /// @returns the best bid price and quantity.
  /// The inside is always at the top_levels_
  half_quote best_quote() const {
    if (tk_inside_ ==
        price_levels(price4_t(0), side<compare_t>::empty_quote_price())) {
      return side<compare_t>::empty_quote();
    }
    auto rel_px = side<compare_t>::level_to_relative(tk_begin_top_, tk_inside_);
    auto px_inside = level_to_price<price4_t>(tk_inside_);
    return half_quote(px_inside, top_levels_[rel_px]);
  }

  /// @returns the worst bid price and quantity.
  /// Check if the side is empty first (return empty_quote in this case)
  /// The worst price is at the bottom_levels (if not empty),
  /// at the top_levels otherwise.
  half_quote worst_quote() const {
    if (tk_inside_ ==
        price_levels(price4_t(0), side<compare_t>::empty_quote_price())) {
      return side<compare_t>::empty_quote(); // empty side
    }
    if (not bottom_levels_.empty()) {
      // ... worst price is at the bottom_levels_
      auto i = bottom_levels_.rbegin(); // get the worst bottom_levels price
      auto px_worst = level_to_price<price4_t>(i->first);
      return half_quote(px_worst, i->second);
    }
    // ... worst price at the top_levels_
    auto rel_worst = relative_worst_top_level();
    auto tk_worst =
        side<compare_t>::relative_to_level(tk_begin_top_, rel_worst);
    auto px_worst = level_to_price<price4_t>(tk_worst);
    return half_quote(px_worst, top_levels_[rel_worst]);
  }

  /// @returns the number of levels with non-zero quantity for the order side.
  /// This is the size of bottom_levels_ plus all non zero top_levels values
  std::size_t count() const {
    return bottom_levels_.size() + top_levels_count();
  }

  /**
   * Add a price and quantity to the side order book.
   * - if px is worse than the px_begin_top_ then px goes to bottom_levels
   * - if px is better than the inside then changes inside, redefines limits
   * if needed (moving the tail to bottom_levels if any)
   * - finally, updates qty at the relative(px)
   *
   * @param px the price of the new order
   * @param qty the quantity of the new order
   * @returns true if the inside changed
   *
   * @throw feed_error px out of valid range, or qty <= 0
   */
  bool add_order(price4_t px, int qty) {
    validate_add_order_params(qty, px);
    // get px price levels
    auto tk_px = price_levels(price4_t(0), px);
    // check if tk_px is worse than the first price of the top_levels_
    if (side<compare_t>::better_level(tk_begin_top_, tk_px)) {
      // emplace the price at the bottom_levels, and return (false)
      auto emp_tup = bottom_levels_.emplace(tk_px, 0);
      emp_tup.first->second += qty;
      return false;
    }
    // check if tk_px is equal or better than the current inside
    if (not side<compare_t>::better_level(tk_inside_, tk_px)) {
      // ... check if limit redefine is needed
      if (not side<compare_t>::better_level(tk_end_top_, tk_px)) {
        // get the new limits based on the new inside tk_px
        auto limits = side<compare_t>::limit_top_prices(tk_px, max_size_ / 2);
        // move the tail [px_begin_top_, new px_begin_top_) to bottom_levels_
        move_top_to_bottom(std::get<0>(limits));
        // ... redefine the limits
        tk_begin_top_ = std::get<0>(limits);
        tk_end_top_ = std::get<1>(limits);
      }
      // if the tk_px inside changed, updated
      if (tk_inside_ != tk_px) {
        tk_inside_ = tk_px;
      }
      // update top_levels_
      auto rel_px =
          side<compare_t>::level_to_relative(tk_begin_top_, tk_inside_);
      top_levels_[rel_px] += qty;
      return true; // the inside changed
    }
    // is a top_levels change different than the inside
    // udates top_levels_ with new qty
    auto rel_px = side<compare_t>::level_to_relative(tk_begin_top_, tk_px);
    top_levels_[rel_px] += qty;
    return false;
  }

  /**
   * Reduce the quantity for a given price.
   *
   * @param px the price of the order that was reduced
   * @param qty the quantity reduced in the order
   * @returns true if the inside changed
   *
   * @throw feed_error qty <= 0
   * @throw feed_error px should be in bottom_levels_ but is empty
   * @throw feed_error trying to reduce a non-existing bottom_levels price
   * @throw feed_error trying to reduce a non-existing top_levels price
   * @throw feed_error trying to reduce a price better than px_inside_
   *
   * Checks and handles if px is a bottom_level_ price
   * Checks if top_levels_ is empty after reducing
   * If so, limits have to be redefined, and tail moved into top_levels
   */
  bool reduce_order(price4_t px, int qty) {
    validate_add_order_params(qty);
    // get px price level
    auto tk_px = price_levels(price4_t(0), px);
    // check and handles if it is a bottom_level_ price
    if (side<compare_t>::better_level(tk_begin_top_, tk_px)) {
      auto price_it = bottom_levels_.find(tk_px);
      if (price_it == bottom_levels_.end()) {
        raise_exception(
            "array_based_book_side::reduce_order."
            " Trying to reduce non-existing bottom_levels_price.",
            tk_begin_top_, px, qty);
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
    if (side<compare_t>::better_level(tk_px, tk_inside_)) {
      raise_exception(
          "array_based_book_side::reduce_order."
          " Trying to reduce a non-existing top_levels_ price"
          " (better px_inside).",
          tk_begin_top_, px, qty);
    }
    // get px relative position
    auto rel_px = side<compare_t>::level_to_relative(tk_begin_top_, tk_px);
    if (top_levels_[rel_px] == 0) {
      raise_exception(
          "array_based_book_side::reduce_order."
          " Trying to reduce a non-existing top_levels_ price"
          " (top_levels_[rel_px] == 0).",
          tk_begin_top_, tk_inside_, rel_px, px, qty);
    }

    // ... reduce the quantity ...
    top_levels_[rel_px] -= qty;
    if (top_levels_[rel_px] < 0) {
      // ... this is "Not Good[tm]", somehow we missed an order or
      // processed a delete twice ...
      JB_LOG(warning) << "negative quantity in order book";
      top_levels_[rel_px] = 0; // cant't be negative
    }
    // ... if it is not the inside we are done
    if (tk_px != tk_inside_) {
      return false;
    }
    // Is inside, ... now check if it was removed
    if (top_levels_[rel_px] == 0) {
      // gets the new inside (if any)
      tk_inside_ = next_best_price_level();
      if (tk_inside_ ==
          price_levels(price4_t(0), side<compare_t>::empty_quote_price())) {
        // last top_levels_ price was removed...
        // ... get the new inside from the bottom_levels
        if (not bottom_levels_.empty()) {
          tk_inside_ = bottom_levels_.begin()->first;
        }
        // redefine limits
        auto limits =
            side<compare_t>::limit_top_prices(tk_inside_, max_size_ / 2);
        tk_begin_top_ = std::get<0>(limits);
        tk_end_top_ = std::get<1>(limits);
        // move tail prices from bottom_levels (tk_begin_top_ {, .begin()})
        move_bottom_to_top();
      }
    }
    return true; // it is an inside change
  }

  /**
   * Testing hook.
   * @returns true is side is ascending
   * To test different implementations for buy and sell sides
   */
  bool is_ascending() const {
    return side<compare_t>::ascending;
  }

private:
  /**
   * @returns relative position of the worst valid price at top_levels.
   * @throw feed_error top_levels_ is empty.
   */
  std::size_t relative_worst_top_level() const {
    return std::distance(
        top_levels_.begin(),
        std::find_if(top_levels_.begin(), top_levels_.end(), [](auto x) {
          return x != 0;
        }));
  }

  /// @returns number of valid prices (>0) at top_levels_
  std::size_t top_levels_count() const {
    // counts from 0 .. relative position of px_inside_
    if (tk_inside_ ==
        price_levels(price4_t(0), side<compare_t>::empty_quote_price())) {
      return 0; // empty side
    }
    auto rel_px = side<compare_t>::level_to_relative(tk_begin_top_, tk_inside_);
    int result = 0;
    for (std::size_t i = 0; i != rel_px + 1; ++i) {
      if (top_levels_[i] != 0) {
        result++;
      }
    }
    return result;
  }

  /**
   * Move prices from tk_begin_top_ to tk_max (excluded)
   * out of top_levels_ to bottom_levels_.
   *
   * @param tk_max first limit price level that is not moved out.
   *
   * Inserts the prices into bottom_levels_
   * Shift top_levels_ prices in order for rel_max to become relative 0
   * Clear (value = 0) former relative position of moved prices
   */
  void move_top_to_bottom(std::size_t const tk_max) {
    JB_ASSERT_THROW(not side<compare_t>::better_level(tk_begin_top_, tk_max));
    // if tk_max is better than tk_inside_
    if (side<compare_t>::better_level(tk_max, tk_inside_)) {
      // ... all top_prices_ have to be moved out
      // valid prices are from 0 .. relative (tk_inside_) included
      auto rel_inside =
          side<compare_t>::level_to_relative(tk_begin_top_, tk_inside_);
      for (std::size_t i = 0; i != rel_inside + 1; ++i) {
        // move out and clear the price (if valid)
        if (top_levels_[i] != 0) {
          auto tk_i = side<compare_t>::relative_to_level(tk_begin_top_, i);
          bottom_levels_.emplace(tk_i, top_levels_[i]);
          top_levels_[i] = 0;
        }
      }
      return;
    }
    // tk_max is worse than tk_inside
    // prices to move out are from 0.. relative (tk_max) excluded
    auto rel_tk_max = side<compare_t>::level_to_relative(tk_begin_top_, tk_max);
    for (std::size_t i = 0; i != rel_tk_max; ++i) {
      // ... move out the price i (if valid)
      if (top_levels_[i] != 0) {
        auto tk_i = side<compare_t>::relative_to_level(tk_begin_top_, i);
        bottom_levels_.emplace(tk_i, top_levels_[i]);
        top_levels_[i] = 0;
      }
    }
    // shift price down rel_tk_max positions
    // from tk_max to tk_inside both included...
    // ...(the are no better prices than tk_inside_)
    auto rel_tk_inside =
        side<compare_t>::level_to_relative(tk_begin_top_, tk_inside_);
    std::size_t j = 0;
    for (std::size_t i = rel_tk_max; i <= rel_tk_inside; ++i, ++j) {
      if (top_levels_[i] != 0) {
        top_levels_[j] = top_levels_[i];
        top_levels_[i] = 0;
      }
    }
  }

  /**
   * Move relative prices from best price to tk_begin_top* (included)
   * out of bottom_levels_ to top_levels_.
   * *tk_begin_top_ is updated before calling this function.
   */
  void move_bottom_to_top() {
    if (bottom_levels_.empty()) {
      return; // nothing to move in
    }
    // traverse all bottom_levels_ prices worse than or equal to tk_begin_top_
    auto le = bottom_levels_.begin();
    for (/* */; le != bottom_levels_.end(); ++le) {
      auto tk_le = le->first;
      if (not side<compare_t>::better_level(tk_begin_top_, tk_le)) {
        // move price to top_levels_
        auto rel_px = side<compare_t>::level_to_relative(tk_begin_top_, tk_le);
        top_levels_[rel_px] = le->second;
      } else {
        // no more good prices to move...
        break;
      }
    }
    // erase the moved prices..
    bottom_levels_.erase(bottom_levels_.begin(), le);
  }

  /**
   * @returns the best next price level from the inside.
   * (return empty quote price if none available).
   */
  std::size_t next_best_price_level() const {
    // begins checking below the current inside
    auto rel_inside =
        side<compare_t>::level_to_relative(tk_begin_top_, tk_inside_);
    if (rel_inside == 0) {
      return price_levels(price4_t(0), side<compare_t>::empty_quote_price());
    }
    do {
      rel_inside--;
      if (top_levels_[rel_inside] != 0) {
        // got the next one...
        return side<compare_t>::relative_to_level(tk_begin_top_, rel_inside);
      }
    } while (rel_inside != 0);
    return price_levels(price4_t(0), side<compare_t>::empty_quote_price());
  }

  /// @returns pair of price levels that are rel worse
  /// and better than tk_px, limited to valid price levels
  /// if tk_px is at the limit, return and empty quote values
  static auto get_limits(std::size_t const tk_px, std::size_t const rel) {
    auto const tk_empty =
        price_levels(price4_t(0), side<compare_t>::empty_quote_price());
    if (tk_px == tk_empty) {
      return std::make_pair(tk_empty, tk_empty);
    }
    auto const level_max = price_levels(price4_t(0), empty_offer_price());

    auto tk_low = tk_px <= rel ? 0 : tk_px - rel;
    auto tk_low_base = tk_low + rel >= level_max ? level_max : tk_low + rel;
    auto tk_high =
        tk_low_base + rel >= level_max ? level_max : tk_low_base + rel;
    auto tk_high_base = tk_high <= rel ? 0 : tk_high - rel;

    if (tk_low_base != tk_high_base) {
      tk_low = tk_high_base <= rel ? 0 : tk_high_base - rel;
    }
    return std::make_pair(tk_low, tk_high);
  }

  /// template specialization struct to handle differences between BUY and SELL
  /// version SELL side.
  template <typename ordering, class DUMMY = void>
  struct side {
    static bool constexpr ascending = false;

    /// @returns an empty offer.
    static half_quote empty_quote() {
      return empty_offer();
    }

    /// @returns empty offer price repesentation.
    static auto empty_quote_price() {
      return empty_offer_price();
    }

    // @returns true if tk1 is less then tk2.
    static bool better_level(std::size_t const tk1, std::size_t const tk2) {
      return tk1 < tk2;
    }

    // @returns pair a new limits
    static auto
    limit_top_prices(std::size_t const tk_px, std::size_t const rel) {
      auto lim = get_limits(tk_px, rel);
      return std::make_pair(std::get<1>(lim), std::get<0>(lim));
    }

    /// returns@ price level rel positions less than tk_ini
    static auto
    relative_to_level(std::size_t const tk_ini, std::size_t const rel) {
      return tk_ini - rel;
    }

    /// @returns relative position of tk_px compare with tk_ini
    static auto
    level_to_relative(std::size_t const tk_ini, std::size_t const tk_px) {
      // check tk_px is in range (less than tk_ini_)
      JB_ASSERT_THROW(tk_px <= tk_ini);
      return tk_ini - tk_px;
    }
  };

  /// version BUY side
  template <class DUMMY>
  struct side<std::greater<std::size_t>, DUMMY> {
    static bool constexpr ascending = true;

    /// @returns an empty bid.
    static half_quote empty_quote() {
      return empty_bid();
    }

    /// @returns empty bid price representation.
    static auto empty_quote_price() {
      return empty_bid_price();
    }

    // @returns true if tk1 is greater then tk2.
    static bool better_level(std::size_t const tk1, std::size_t const tk2) {
      return tk1 > tk2;
    }

    // @returns pair a new limits.
    static auto
    limit_top_prices(std::size_t const tk_px, std::size_t const rel) {
      return get_limits(tk_px, rel);
    }

    /// returns@ price level rel positions greater than tk_ini.
    static auto
    relative_to_level(std::size_t const tk_ini, std::size_t const rel) {
      return tk_ini + rel;
    }

    /// @returns relative position of tk_px compare with tk_ini.
    static auto
    level_to_relative(std::size_t const tk_ini, std::size_t const tk_px) {
      // check tk_px is in range (better than tk_ini_)
      JB_ASSERT_THROW(tk_px >= tk_ini);
      return tk_px - tk_ini;
    }
  };

private:
  /// top_levels_ max size
  std::size_t max_size_;

  /// the best relative prices and quantity
  std::vector<int> top_levels_;

  /// the worst (tail) price level and quantity
  std::map<std::size_t, int, compare_t> bottom_levels_;

  /// price level the inside
  std::size_t tk_inside_;

  /// worst price level on the top_levels_ range
  std::size_t tk_begin_top_;

  /// one price level past-the-best price in top_levels_ range
  std::size_t tk_end_top_;
};

} // namespace itch5
} // namespace jb

#endif // jb_itch5_array_based_order_book_hpp
