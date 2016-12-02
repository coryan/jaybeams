#ifndef jb_itch5_order_book_hpp
#define jb_itch5_order_book_hpp

#include <jb/itch5/buy_sell_indicator.hpp>
#include <jb/itch5/price_field.hpp>
#include <jb/feed_error.hpp>
#include <jb/log.hpp>

#include <functional>
#include <utility>

namespace jb {
namespace itch5 {

/**
 * Define the types of buy and sell sides data structure.
 *
 * It is used as template parameter book_type of the
 * template class order_book:
 * - usage: jb::itch5::order_book<jb::itch5::map_price>
 */
struct map_price {
  using buys_t = std::map<price4_t, int, std::greater<price4_t>>;
  using sells_t = std::map<price4_t, int, std::less<price4_t>>;
};

/// A simple representation for price + quantity
using half_quote = std::pair<price4_t, int>;
using book_depth_t = unsigned long int;

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
 *
 * This is a template class.
 * @tparam book_type defines data structure type of price book,
 * both sides buy and sell. Must be compatible with jb::itch5::map_price
 */

template <typename book_type>
class order_book {
public:
  /// Initialize an empty order book.
  order_book() {
  }

  /// The value used to represent an empty bid
  static half_quote empty_bid() {
    return half_quote(price4_t(0), 0);
  }
  /// The value used to represent an empty offer
  static half_quote empty_offer() {
    return half_quote(max_price_field_value<price4_t>(), 0);
  }

  //@{
  /**
   * @name Accessors
   */

  /// @returns the best bid price and quantity
  half_quote best_bid() const {
    if (buy_.empty()) {
      return empty_bid();
    }
    auto i = buy_.begin();
    return half_quote(i->first, i->second);
  }

  /// @returns the worst bid price and quantity
  half_quote worst_bid() const {
    if (buy_.empty()) {
      return empty_bid();
    }
    auto i = buy_.rbegin();
    return half_quote(i->first, i->second);
  }

  /// @returns the best offer price and quantity
  half_quote best_offer() const {
    if (sell_.empty()) {
      return empty_offer();
    }
    auto i = sell_.begin();
    return half_quote(i->first, i->second);
  }

  /// @returns the worst offer price and quantity
  half_quote worst_offer() const {
    if (sell_.empty()) {
      return empty_offer();
    }
    auto i = sell_.rbegin();
    return half_quote(i->first, i->second);
  }

  /// @returns the number of levels with non-zero quantity for the BUY side.
  std::size_t buy_count() const {
    return buy_.size();
  }

  /// @returns the number of levels with non-zero quantity for the SELL side.
  std::size_t sell_count() const {
    return sell_.size();
  }

  /// @returns the book depth, this is the number of price levels on
  /// this order book.
  book_depth_t get_book_depth() const {
    return buy_count() + sell_count();
  };
  //@}

  /**
   * Handle a new order.
   *
   * Update the quantity at the right price level in the correct
   * side of the book.
   *
   * @param side whether the order is a buy or a sell
   * @param px the price of the order
   * @param qty the quantity of the order
   * @return true if the inside changed
   */
  bool handle_add_order(buy_sell_indicator_t side, price4_t px, int qty) {
    if (side == buy_sell_indicator_t('B')) {
      return handle_add_order(buy_, px, qty);
    }
    return handle_add_order(sell_, px, qty);
  }

  /**
   * Handle an order reduction, which includes executions,
   * cancels and replaces.
   *
   * @param side whether the order is a buy or a sell
   * @param px the price of the order
   * @param reduced_qty the executed quantity of the order
   * @returns true if the inside changed
   */
  bool handle_order_reduced(
      buy_sell_indicator_t side, price4_t px, int reduced_qty) {
    if (side == buy_sell_indicator_t('B')) {
      return handle_order_reduced(buy_, px, reduced_qty);
    }
    return handle_order_reduced(sell_, px, reduced_qty);
  }

private:
  /**
   * Refactor handle_add_order()
   *
   * @tparam book_side the type used to represent the buy or sell side
   * of the book
   * @param side the side of the book to update
   * @param px the price of the new order
   * @param qty the quantity of the new order
   * @returns true if the inside changed
   */
  template <typename book_side>
  bool handle_add_order(book_side& side, price4_t px, int qty) {
    auto emp_tup = side.emplace(px, 0);
    emp_tup.first->second += qty;
    return emp_tup.first == side.begin();
  }

  /**
   * Refactor handle_order_reduce()
   *
   * @tparam book_side the type used to represent the buy or sell side
   * of the book
   * @param side the side of the book to update
   * @param px the price of the order that was reduced
   * @param reduced_qty the quantity reduced in the order
   * @returns true if the inside changed
   */
  template <typename book_side>
  bool handle_order_reduced(book_side& side, price4_t px, int reduced_qty) {
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
    bool inside_change = (price_it == side.begin());
    if (price_it->second <= 0) {
      side.erase(price_it);
    }
    return inside_change;
  }

private:
  typename book_type::buys_t buy_;
  typename book_type::sells_t sell_;
};

} // namespace itch5
} // namespace jb

#endif // jb_itch5_order_book_hpp
