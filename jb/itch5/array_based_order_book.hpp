#ifndef jb_itch5_array_based_order_book_hpp
#define jb_itch5_array_based_order_book_hpp

#include <jb/itch5/map_based_order_book.hpp>
#include <jb/itch5/price_field.hpp>
#include <jb/feed_error.hpp>
#include <jb/log.hpp>

#include <functional>
#include <map>
#include <utility>

namespace jb {
namespace itch5 {

/// A simple representation for price + quantity
using half_quote = std::pair<price4_t, int>;

/**
 *  Represent one side of the book.
 *
 * @tparam compare It sorts the side order book
 */
template <typename compare_t>
class array_based_book_side {
public:
  /// Initializes an empty side order book
  array_based_book_side() {
  }

  /// @returns an empty bid or offer based on compare function
  /// empty bid for less, empty offer for greater.
  half_quote empty_quote() const {
    if (compare_(price4_t(1), price4_t(0))) {
      return empty_bid();
    }
    return empty_offer();
  }

  /// @returns the best bid price and quantity
  half_quote best_quote() const {
    if (levels_.empty()) {
      return empty_quote();
    }
    auto i = levels_.begin();
    return half_quote(i->first, i->second);
  }

  /// @returns the worst bid price and quantity
  half_quote worst_quote() const {
    if (levels_.empty()) {
      return empty_quote();
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
      throw jb::feed_error(
          "on array_based_order_book. "
          "Trying to reduce a non-existing price level");
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
   * @returns true is px1 < px2
   * To validate different implementations for buy and sell sides
   */
  bool check_less(price4_t const& px1, price4_t const& px2) const {
    return compare_(px1, px2);
  }

private:
  std::map<price4_t, int, compare_t> levels_;
  compare_t compare_;
};

/**
 * Define the types of buy and sell sides data structure.
 *
 * It is used as template parameter book_type of the
 * template class order_book:
 * - usage: jb::itch5::order_book<jb::itch5::array_based_order_book>
 */
struct array_based_order_book {
  using buys_t = array_based_book_side<std::greater<price4_t>>;
  using sells_t = array_based_book_side<std::less<price4_t>>;
};

} // namespace itch5
} // namespace jb

#endif // jb_itch5_array_based_order_book_hpp
