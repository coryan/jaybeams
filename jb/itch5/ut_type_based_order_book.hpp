#ifndef jb_itch5_ut_type_based_order_book_hpp
#define jb_itch5_ut_type_based_order_book_hpp

#include <jb/itch5/map_based_order_book.hpp>

#include <boost/test/unit_test.hpp>

namespace jb {
namespace itch5 {
namespace testing {

/**
 * side_type class trivial member tests.
 * @tparam side_type Side type to be tested
 *
 * Uses testing hook check_less to know if tested is buy or sell side
 */
template <typename side_type>
void test_side_type_trivial(side_type& tested) {
  using jb::itch5::price4_t;
  auto is_less = tested.check_less(price4_t(1), price4_t(0));
  auto actual = tested.best_quote();
  if (is_less) {
    BOOST_CHECK_EQUAL(actual.first, price4_t(0));
    BOOST_CHECK_EQUAL(actual.second, 0);
  } else {
    BOOST_CHECK_EQUAL(actual.first, price4_t(200000UL * 10000));
    BOOST_CHECK_EQUAL(actual.second, 0);
  }

  actual = tested.worst_quote();
  if (is_less) {
    BOOST_CHECK_EQUAL(actual.first, price4_t(0));
    BOOST_CHECK_EQUAL(actual.second, 0);
  } else {
    BOOST_CHECK_EQUAL(actual.first, price4_t(200000UL * 10000));
    BOOST_CHECK_EQUAL(actual.second, 0);
  }

  //  book_depth should be 0
  BOOST_CHECK_EQUAL(tested.count(), 0);
}

/**
 * Test side type error handling.
 * @tparam side_type Side to be tested
 */
template <typename side_type>
void test_side_type_errors(side_type& tested) {
  using jb::itch5::price4_t;

  int diff;
  if (tested.check_less(price4_t(1), price4_t(0))) {
    diff = 10000;
  } else {
    diff = -10000;
  }

  /// Uses Testing hooks to increase coverage
  /// test with an empty side
  BOOST_CHECK_THROW(
      tested.test_relative_worst_top_level(), jb::feed_error);

  // Add two orders to the book ...
  (void)tested.add_order(price4_t(100000), 100);
  (void)tested.add_order(price4_t(100000 + diff), 200);

  /** Uses Testing hooks to increase coverage
   * px worse than the low limit
   */
  if (diff < 0) {
    // buy side
    BOOST_CHECK_THROW(
	tested.test_price_to_relative(price4_t(1000000)), jb::feed_error);
    /// to test with a px worse than px_begin_top
    BOOST_CHECK_THROW(
	tested.test_move_top_to_bottom(price4_t(1000000)), jb::feed_error);
  } else {
    // sell side
    BOOST_CHECK_THROW(
	tested.test_price_to_relative(price4_t(100)), jb::feed_error);
    /// to test with a px worse than px_begin_top
    BOOST_CHECK_THROW(
	tested.test_move_top_to_bottom(price4_t(100)), jb::feed_error);
  }

  // ... check the best quote ...
  auto actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000 + diff));
  BOOST_CHECK_EQUAL(actual.second, 200);

  // ... remove the first order, once should work, the second time
  // should fail ...
  tested.reduce_order(price4_t(100000), 100);
  BOOST_CHECK_THROW(tested.reduce_order(price4_t(100000), 100), jb::feed_error);

  // ... check the best quote again ...
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000 + diff));
  BOOST_CHECK_EQUAL(actual.second, 200);

  // ... add order above the limit price
  BOOST_CHECK_THROW(tested.add_order(price4_t(-1), 200), jb::feed_error);

  // ... reduce order with negative qty
  BOOST_CHECK_THROW(
      tested.reduce_order(price4_t(100000 + diff), -100), jb::feed_error);

  // ... reduce a non existing price better than the inside
  BOOST_CHECK_THROW(
      tested.reduce_order(price4_t(100000 + 2*diff), 100), jb::feed_error);
  
  // ... reduce non existing order on an empty bottom levels
  if (diff < 0) {
    // buy side
    BOOST_CHECK_THROW(
      tested.reduce_order(price4_t(100), 100), jb::feed_error);
    // add one, so no empty now...
    (void)tested.add_order(price4_t(101), 100);
    // try to reduce again
    BOOST_CHECK_THROW(
      tested.reduce_order(price4_t(100), 100), jb::feed_error);
    // and finally reduce the existing one but over quantity, should work
    (void)tested.reduce_order(price4_t(101), 200);
  } else {
    // sell side
    BOOST_CHECK_THROW(
      tested.reduce_order(price4_t(1000000), 100), jb::feed_error);
    // add one, so no empty now...
    (void)tested.add_order(price4_t(1000100), 100);
    // try to reduce again
    BOOST_CHECK_THROW(
      tested.reduce_order(price4_t(1000000), 100), jb::feed_error); 
    // and finally reduce the existing one but over quantity, should work
    (void)tested.reduce_order(price4_t(1000100), 200);
 }
}

template <typename side_type>
void test_side_type_add_reduce(side_type& tested) {
  using jb::itch5::price4_t;

  int diff;
  if (tested.check_less(price4_t(0), price4_t(1))) {
    diff = -10000;
  } else {
    diff = 10000;
  }
  int base_p = 4000000;

  // Add a new order ...
  auto r = tested.add_order(price4_t(base_p), 100);

  // .. best quote should change ...
  auto actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(base_p));
  BOOST_CHECK_EQUAL(actual.second, 100);
  actual = tested.worst_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(base_p));
  BOOST_CHECK_EQUAL(actual.second, 100);
  // handler should return true... it is an inside change
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.count(), 1);

  // ... adding below the best quote has no effect ...
  r = tested.add_order(price4_t(base_p - diff), 300);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(base_p));
  BOOST_CHECK_EQUAL(actual.second, 100);
  actual = tested.worst_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(base_p - diff));
  BOOST_CHECK_EQUAL(actual.second, 300);
  // handler should return false
  BOOST_CHECK_EQUAL(r, false);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.count(), 2);

  // ... update at the best quote increases the qty ...
  r = tested.add_order(price4_t(base_p), 400);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(base_p));
  BOOST_CHECK_EQUAL(actual.second, 500);
  // handler should return true... it is an inside change
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should not be incremented
  BOOST_CHECK_EQUAL(tested.count(), 2);

  // ... a better price changes both price and qty ...
  r = tested.add_order(price4_t(base_p + diff), 200);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(base_p + diff));
  BOOST_CHECK_EQUAL(actual.second, 200);
  // handler should return true... it is an inside change
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.count(), 3);

  // ... decrease below the bid has no effect ...
  r = tested.reduce_order(price4_t(base_p), 400);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(base_p + diff));
  BOOST_CHECK_EQUAL(actual.second, 200);
  // handler should return false
  BOOST_CHECK_EQUAL(r, false);
  // .. and the book_depth should not be decremented
  BOOST_CHECK_EQUAL(tested.count(), 3);

  // ... even when it is over the existing quantity ...
  r = tested.reduce_order(price4_t(base_p), 200);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(base_p + diff));
  BOOST_CHECK_EQUAL(actual.second, 200);
  // handler should return false
  BOOST_CHECK_EQUAL(r, false);
  // .. and the book_depth should be decremented
  BOOST_CHECK_EQUAL(tested.count(), 2);

  // ... deleting the best quote uncovers the best price ...
  r = tested.reduce_order(price4_t(base_p + diff), 200);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(base_p - diff));
  BOOST_CHECK_EQUAL(actual.second, 300);
  // handler should return true... it is an inside change
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should be decremented
  BOOST_CHECK_EQUAL(tested.count(), 1);

  // ... deleting the remaining price takes the book depth to 0
  r = tested.reduce_order(price4_t(base_p - diff), 300);
  actual = tested.best_quote();
  auto emp = tested.empty_quote(); // gets empty_quoted already tested
  BOOST_CHECK_EQUAL(actual.first, emp.first);
  BOOST_CHECK_EQUAL(actual.second, emp.second);
  // handler should return true... it is an inside change
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should be decremented
  BOOST_CHECK_EQUAL(tested.count(), 0);
}

/**
*order_book type trivial test.
*@tparam order_book type based order book to be tested
*/
template <typename order_book>
void test_order_book_type_trivial(order_book& tested) {
  typename order_book::buys_t buy_test(10000);
  test_side_type_trivial(buy_test);

  typename order_book::sells_t sell_test(10000);
  test_side_type_trivial(sell_test);
}

/**
 * order_book type error handling test.
 * @tparam order_book type based order book to be tested
 */
template <typename order_book>
void test_order_book_type_errors(order_book& tested) {
  typename order_book::buys_t buy_test(10000);
  test_side_type_errors(buy_test);

  typename order_book::sells_t sell_test(10000);
  test_side_type_errors(sell_test);
}

/**
 * order_book type add and reduce handling test.
 * @tparam order_book type based order book to be tested
 */
template <typename order_book>
void test_order_book_type_add_reduce(order_book& tested) {
  typename order_book::buys_t buy_test(10000);
  test_side_type_add_reduce(buy_test);

  typename order_book::sells_t sell_test(10000);
  test_side_type_add_reduce(sell_test);
}

} // namespace testing
} // namespace itch5
} // namespace jb

#endif // jb_itch5_ut_type_based_order_book_hpp
