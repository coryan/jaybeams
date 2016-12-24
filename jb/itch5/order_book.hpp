#ifndef jb_itch5_order_book_hpp
#define jb_itch5_order_book_hpp

#include <jb/itch5/array_based_order_book.hpp>
#include <jb/itch5/buy_sell_indicator.hpp>
#include <jb/itch5/map_based_order_book.hpp>
#include <jb/itch5/price_field.hpp>
#include <jb/itch5/quote_defaults.hpp>
#include <jb/config_object.hpp>

#include <type_traits>

namespace jb {
namespace itch5 {

/// Number of prices on a side order book
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
 * This class encapsulates the order book data structure as well as its
 * configuration.
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
 * both sides buy and sell.
 * Must be compatible with jb::itch5::map_based_order_book
 *
 * @param cfg book_type config
 */

template <typename book_type>
class order_book {
public:
  using book_type_config = typename book_type::config;

  /// Initialize an empty order book.
  explicit order_book(book_type_config const& cfg)
      : buy_(cfg)
      , sell_(cfg) {
  }

  //@{
  /**
   * @name Accessors
   */

  /// @returns the best bid price and quantity
  half_quote best_bid() const {
    return buy_.best_quote();
  }

  /// @returns the worst bid price and quantity
  half_quote worst_bid() const {
    return buy_.worst_quote();
  }

  /// @returns the best offer price and quantity
  half_quote best_offer() const {
    return sell_.best_quote();
  }

  /// @returns the worst offer price and quantity
  half_quote worst_offer() const {
    return sell_.worst_quote();
  }

  book_depth_t buy_count() const {
    return buy_.count();
  }
  book_depth_t sell_count() const {
    return sell_.count();
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
      return buy_.add_order(px, qty);
    }
    return sell_.add_order(px, qty);
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
      return buy_.reduce_order(px, reduced_qty);
    }
    return sell_.reduce_order(px, reduced_qty);
  }

private:
  typename book_type::buys_t buy_;
  typename book_type::sells_t sell_;
};

} // namespace itch5
} // namespace jb

#endif // jb_itch5_order_book_hpp
