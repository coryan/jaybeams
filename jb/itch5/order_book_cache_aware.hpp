#ifndef jb_itch5_order_book_cache_aware_hpp
#define jb_itch5_order_book_cache_aware_hpp

#include <jb/itch5/buy_sell_indicator.hpp>
#include <jb/itch5/order_book_def.hpp>
#include <jb/itch5/price_field.hpp>
#include <jb/feed_error.hpp>
#include <jb/log.hpp>

#include <functional>
#include <map>
#include <utility>

namespace jb {
namespace itch5 {

/// A simple representation for price + quantity
typedef std::pair<price4_t, int> half_quote;

/// A simple representation for price range {lower_price .. higher_price}
typedef std::pair<price4_t, price4_t> price_range_t;

/// result when adding or updating orders
/// (number of tick of the inside change, price levels moved to/from tail)
typedef std::pair<tick_t, int> order_book_change_t;

/// $1.00 in ticks
const int PX_DOLLAR_TICK = 10000;

/**
 * Maintain the ITCH-5.0 order book for a single security.
 *
 * ITCH-5.0, like other market data feeds provide order-by-order
 * detail, that is, the feed includes a message for each order
 * received by the exchange, as well as the changes to these
 * orders, i.e. when the execute, when their quantity (and/or price)
 * is modified, and when they are canceled..  Such feeds are sometimes
 * called Level III feeds.  Typically only orders that do not
 * immediately execute in full are included in the feed.
 *
 * There is substantial variation in the format of the messages, and
 * some variation as to whether executions are represented differently
 * than a partial cancel, and whether changes in price are allowed or
 * create new order ids.
 *
 * This class receives a stream of (unnormalized) ITCH-5.0 messages
 * for a single security, and organizes the orders in a
 * book, i.e. a  data structure where orders at the same price are
 * grouped together, and one can quickly ask:
 *
 * - What is the best bid (highest price of BUY orders) and what is
 * the total quantity available at that price?
 * - What is the best offer (lowest price of SELL orders) and what is
 * the total quantity available at that price?
 */
class order_book_cache_aware {
public:
  /// Initialize an empty order book.
  order_book_cache_aware() {
  }

  /// Return the tick offset
  static tick_t tick_offset();
  /// Set the tick offset
  static void tick_offset(tick_t const tick);

  /// Return the best bid price and quantity
  half_quote best_bid() const;
  /// Return the best offer price and quantity
  half_quote best_offer() const;
  /// Return the best bid price and quantity
  price4_t best_bid_price() const {
    return std::get<0>(best_bid());
  }
  /// Return the best offer price and quantity
  price4_t best_offer_price() const {
    return std::get<0>(best_offer());
  }

  /// The value used to represent an empty bid
  static half_quote empty_bid() {
    return half_quote(price4_t(0), 0);
  }
  /// The value used to represent an empty offer
  static half_quote empty_offer() {
    return half_quote(max_price_field_value<price4_t>(), 0);
  }

  /// The value used to represent a default bid price range
  price_range_t default_bid_price_range() {
    return price_range(buy_sell_indicator_t('B'), price4_t(100 * tick_off_));
  }
  /// The value used to represent a default offer price range
  price_range_t default_offer_price_range() {
    return price_range(buy_sell_indicator_t('S'), price4_t(100 * tick_off_));
  }

  /**
   * Return the book depth.
   * This is the number of price levels on this order book.
   */
  book_depth_t get_book_depth() const {
    return buy_.size() + sell_.size();
  }

  /**
 * Handle a new order.
 *
 * Update the quantity at the right price level in the correct
 * side of the book.
 *
 * if the inside price changes the number of ticks of change is reported:
 * 0: no change on the inside price
 * n > 0: new inside price is n ticks better than the last one
 * n < 0: new inside price is n ticks worse than the last one
 *
 * @param side whether the order is a buy or a sell
 * @param px the price of the order
 * @param qty the quantity of the order
 * @return <number of ticks the inside price changed, price levels moved
 * to or from the tail>
 */
  order_book_change_t
  handle_add_order(buy_sell_indicator_t side, price4_t px, int qty) {
    if (side == buy_sell_indicator_t('B')) {
      if (buy_.size() == 0) { // first order on the book
        (void)buy_.emplace(px, qty);
        // set the new price_range around the new px
        buy_price_range_ = price_range(side, px);
        return std::make_pair(0, 0);
      }
      auto tick_change = handle_add_order(buy_, px, qty);
      int num_price_to_tail = 0;
      if (tick_change != 0) {
        num_price_to_tail = side_price_levels(side);
      }
      return std::make_pair(tick_change, num_price_to_tail);
    }
    if (sell_.size() == 0) { // first order on the book
      (void)sell_.emplace(px, qty);
      // set the new price_range around the new px
      sell_price_range_ = price_range(side, px);
      return std::make_pair(0, 0);
    }
    auto tick_change = handle_add_order(sell_, px, qty);
    int num_price_to_tail = 0;
    if (tick_change != 0) {
      num_price_to_tail = side_price_levels(side);
    }
    return std::make_pair(tick_change, num_price_to_tail);
  }

  /**
   * Handle an order reduction, which includes executions,
   * cancels and replaces.
   *
   * @param side whether the order is a buy or a sell
   * @param px the price of the order
   * @param reduced_qty the executed quantity of the order
   * @return int number of ticks the inside price changed
   */
  order_book_change_t
  handle_order_reduced(buy_sell_indicator_t side, price4_t px, int qty) {
    if (side == buy_sell_indicator_t('B')) {
      auto tick_change = handle_order_reduced(buy_, px, qty);
      int num_price_from_tail = 0;
      if (tick_change != 0) {
        num_price_from_tail = side_price_levels(side);
      }
      return std::make_pair(tick_change, num_price_from_tail);
    }
    auto tick_change = handle_order_reduced(sell_, px, qty);
    int num_price_from_tail = 0;
    if (tick_change != 0) {
      num_price_from_tail = side_price_levels(side);
    }
    return std::make_pair(tick_change, num_price_from_tail);
  }

  /// Get the side price range
  price_range_t price_range(buy_sell_indicator_t) const;
  /// Set the side price range
  price_range_t price_range(buy_sell_indicator_t, price4_t);

private:
  /**
   * Refactor handle_add_order()
   *
   * @tparam book_side the type used to represent the buy or sell side
   * of the book
   * @param side the side of the book to update
   * @param px the price of the new order
   * @param qty the quantity of the new order
   * @return int number of ticks the inside price changed
   */
  template <typename book_side>
  int handle_add_order(book_side& side, price4_t px, int qty) {
    auto side_size = side.size();
    auto emp_tup = side.emplace(px, 0);
    emp_tup.first->second += qty;
    // it is just a qty update, not a new price...
    if (side_size == side.size()) {
      return 0;
    }
    // ... it is a new price
    // check if it is a new inside...
    // ... get the tick shift is so
    if (emp_tup.first == side.begin()) {
      auto old_p = ((++side.begin())->first);
      return num_ticks(old_p, px);
    }
    return 0; // no price change at the inside
  }

  /**
   * Refactor handle_order_reduce()
   *
   * @tparam book_side the type used to represent the buy or sell side
   * of the book
   * @param side the side of the book to update
   * @param px the price of the order that was reduced
   * @param reduced_qty the quantity reduced in the order
   * @return int number of ticks the inside price changed
   */
  template <typename book_side>
  int handle_order_reduced(book_side& side, price4_t px, int reduced_qty) {
    auto price_it = side.find(px);
    if (price_it == side.end()) {
      throw jb::feed_error("trying to reduce a non-existing price level");
    }
    // ... reduce the quantity ...
    price_it->second -= reduced_qty;
    if (price_it->second < 0) {
      // ... this is "Not Good[tm]", somehow we missed an order or
      // processed a delete twice ...
      JB_LOG(warning) << "negative quantity in order book";
    }
    // now we can erase this element (pf.first) if qty <=0
    // ... save if it is the inside being removed
    if (price_it->second <= 0) {
      auto is_inside = (price_it == side.begin());
      side.erase(price_it);
      if (side.empty()) {
        return 0; // no need to look for a new price
      }
      // ... now if it was the inside removed ...
      if (is_inside) {
        auto new_p = side.begin()->first;
        return num_ticks(px, new_p);
      }
    }
    return 0;
  }

  /// check if the price is outside the range
  bool check_off_limits(buy_sell_indicator_t const, price4_t const) const;

  /// return the price levels between two prices
  int price_levels(
      buy_sell_indicator_t const, price4_t const, price4_t const) const;

  /// return the price levels the inside moved
  int side_price_levels(buy_sell_indicator_t const);

  /// return the num ticks between two prices
  int num_ticks(price4_t const, price4_t const) const;

  /// get a the price range around a price
  price_range_t price_range(price4_t);

  typedef std::map<price4_t, int, std::greater<price4_t>> buys;
  typedef std::map<price4_t, int, std::less<price4_t>> sells;

  buys buy_;
  sells sell_;

  price_range_t buy_price_range_ = default_bid_price_range();
  price_range_t sell_price_range_ = default_offer_price_range();

  static tick_t tick_off_;
};

} // namespace itch5
} // namespace jb

#endif // jb_itch5_order_book_cache_aware_hpp
