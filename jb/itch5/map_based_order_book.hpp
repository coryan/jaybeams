#ifndef jb_itch5_map_based_order_book_hpp
#define jb_itch5_map_based_order_book_hpp

#include <jb/itch5/price_field.hpp>
#include <jb/itch5/quote_defaults.hpp>
#include <jb/feed_error.hpp>
#include <jb/log.hpp>

#include <functional>
#include <map>
#include <utility>

namespace jb {
namespace itch5 {

template <typename compare_t>
class map_based_book_side;

/**
 * Define the types of buy and sell sides data structure.
 *
 * It is used as template parameter book_type of the
 * template class order_book:
 * - usage: jb::itch5::order_book<jb::itch5::map_based_order_book>
 */
struct map_based_order_book {
  using buys_t = map_based_book_side<std::greater<price4_t>>;
  using sells_t = map_based_book_side<std::less<price4_t>>;
  class config;
};

/**
 * Configure an map_based_order_book config object
 */
class map_based_order_book::config : public jb::config_object {
public:
  config() {
  }
  config_object_constructors(config);
  /// empty
  void validate() const override {
  }
  /* no members */
};

/**
 * Represent one side of the book. Class implementation of struct
 * map_based_order_book buy and side types.
 * @tparam compare_t function object class type to sort the side
 */
template <typename compare_t>
class map_based_book_side {
public:
  /// Initializes an empty side order book
  map_based_book_side(map_based_order_book::config const& cfg) {
  }

  /// @returns the best side price and quantity
  half_quote best_quote() const {
    if (levels_.empty()) {
      return side<compare_t>::empty_quote();
    }
    auto i = levels_.begin();
    return half_quote(i->first, i->second);
  }

  /// @returns the worst side price and quantity
  half_quote worst_quote() const {
    if (levels_.empty()) {
      return side<compare_t>::empty_quote();
    }
    auto i = levels_.rbegin();
    return half_quote(i->first, i->second);
  }

  /// @returns the number of levels with non-zero quantity for the order side.
  std::size_t count() const {
    return levels_.size();
  }

  /**
   * Add a price and quantity to the side order book.
   *
   * @param px the price of the new order
   * @param qty the quantity of the new order
   * @returns true if the inside changed
   */
  bool add_order(price4_t px, int qty) {
    auto emp_tup = levels_.emplace(px, 0);
    emp_tup.first->second += qty;
    return emp_tup.first == levels_.begin();
  }

  /**
   * Reduce the quantity for a given price
   *
   * @param px the price of the order that was reduced
   * @param reduced_qty the quantity reduced in the order
   * @returns true if the inside changed
   */
  bool reduce_order(price4_t px, int reduced_qty) {
    auto price_it = levels_.find(px);
    if (price_it == levels_.end()) {
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
    bool inside_change = (price_it == levels_.begin());
    if (price_it->second <= 0) {
      levels_.erase(price_it);
    }
    return inside_change;
  }

  /**
   * Testing hook.
   * @returns true if side is in ascending order (BUY side)
   * To discriminate different implementations for buy and sell sides
   * during testing.
   */
  bool is_ascending() const {
    return side<compare_t>::ascending;
  }

  /// template specialization struct to handle differences between BUY and SELL
  /// version SELL side
  template <typename ordering, class DUMMY = void>
  struct side {
    static bool constexpr ascending = false;

    /// @returns an empty offer
    static half_quote empty_quote() {
      return empty_offer();
    }
  };

  /// version BUY side
  template <class DUMMY>
  struct side<std::greater<price4_t>, DUMMY> {
    static bool constexpr ascending = true;

    /// @returns an empty bid
    static half_quote empty_quote() {
      return empty_bid();
    }
  };

private:
  std::map<price4_t, int, compare_t> levels_;
};

} // namespace itch5
} // namespace jb

#endif // jb_itch5_map_based_order_book_hpp
