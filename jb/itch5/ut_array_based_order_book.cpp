#include <jb/itch5/array_based_order_book.hpp>
#include <jb/itch5/ut_type_based_order_book.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Trivial verification that array_based_order_book works as expected.
 */
BOOST_AUTO_TEST_CASE(array_based_order_book_trivial) {
  jb::itch5::array_based_order_book tested;
  jb::itch5::testing::test_order_book_type_trivial(tested);
}

/**
 * @test Verify that array_based_order_book handles add and reduce as expected.
 */

BOOST_AUTO_TEST_CASE(array_based_order_book_test) {
  jb::itch5::array_based_order_book tested;
  jb::itch5::testing::test_order_book_type_add_reduce(tested);
}

/**
 * @test Verify that array_based_order_book handles errors as expected.
 */
BOOST_AUTO_TEST_CASE(array_based_order_book_errors) {
  jb::itch5::array_based_order_book tested;
  jb::itch5::testing::test_order_book_type_errors(tested);
}

/**
 * @test Verify that the buy side of jb::itch5::order_book_cache_aware works as
 * expected.
 */
BOOST_AUTO_TEST_CASE(order_book_cache_aware_buy) {
  using jb::itch5::price4_t;

  std::size_t ticks = 5000;
  jb::itch5::array_based_order_book::buys_t tested(2 * ticks);

  // Add a new order ...
  auto r = tested.add_order(price4_t(100000), 100);
  // .. best quote should change
  auto actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);
  // handler should return true... it is the first price set
  BOOST_CHECK_EQUAL(r, true);
  // check the range
  auto rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(5900));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(600000));
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.count(), 1);
  auto rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 1);

  // ... adding below the best bid has no effect ...
  r = tested.add_order(price4_t(99900), 300);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);
  BOOST_CHECK_EQUAL(r, false);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.count(), 2);

  // ... update at the bid increases the qty ...
  r = tested.add_order(price4_t(100000), 400);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 500);
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should not be incremented
  BOOST_CHECK_EQUAL(tested.count(), 2);

  // ... a better price changes both price (+1 ticks) and qty ...
  r = tested.add_order(price4_t(100100), 200);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 200);
  BOOST_CHECK_EQUAL(r, true); // inside moves one tick up
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.count(), 3);

  // ... decrease below the bid has no effect ...
  r = tested.reduce_order(price4_t(100000), 400);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 200);
  BOOST_CHECK_EQUAL(r, false);
  // .. and the book depth should not be decremented
  BOOST_CHECK_EQUAL(tested.count(), 3);

  // ... even when it is over the existing quantity ...
  r = tested.reduce_order(price4_t(100000), 200);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 200);
  BOOST_CHECK_EQUAL(r, false);
  // .. and the book_depth should be decremented
  BOOST_CHECK_EQUAL(tested.count(), 2);

  // ... deleting the best bid uncovers the best price ...
  r = tested.reduce_order(price4_t(100100), 200);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 300);
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should be decremented
  BOOST_CHECK_EQUAL(tested.count(), 1);

  // ... deleting the remaining price takes the book depth to 0
  r = tested.reduce_order(price4_t(99900), 300);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(0));
  BOOST_CHECK_EQUAL(actual.second, 0);
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should be decremented
  BOOST_CHECK_EQUAL(tested.count(), 0);
}

/**
 * @test Verity that the sell side of jb::itch5::order_book_cache_aware works as
 * expected.
 */
BOOST_AUTO_TEST_CASE(order_book_cache_aware_sell) {
  using jb::itch5::price4_t;

  std::size_t ticks = 5000;
  jb::itch5::array_based_order_book::sells_t tested(2 * ticks);

  // Add a new order ...
  auto r = tested.add_order(price4_t(100000), 100);
  // .. the best quote should change
  auto actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);
  BOOST_CHECK_EQUAL(r, true); // first order
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.count(), 1);

  // ... adding above the best offer has no effect ...
  r = tested.add_order(price4_t(100100), 300);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);
  BOOST_CHECK_EQUAL(r, false);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.count(), 2);

  // ... update at the offer increases the qty ...
  r = tested.add_order(price4_t(100000), 400);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 500);
  // handler should return true... it is an inside change
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should not change
  BOOST_CHECK_EQUAL(tested.count(), 2);

  // ... a better price changes both price and qty ...
  r = tested.add_order(price4_t(99900), 200);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 200);
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.count(), 3);

  // ... decrease above the offer has no effect ...
  r = tested.reduce_order(price4_t(100000), 400);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 200);
  BOOST_CHECK_EQUAL(r, false);
  // .. and the book_depth should not change
  BOOST_CHECK_EQUAL(tested.count(), 3);

  // ... even when it is over the existing quantity ...
  r = tested.reduce_order(price4_t(100000), 200);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 200);
  BOOST_CHECK_EQUAL(r, false);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.count(), 2);

  // ... deleting the best offer uncovers the best price ...
  r = tested.reduce_order(price4_t(99900), 200);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 300);
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.count(), 1);

  // ... deleting the remaining price takes the book depth to 0
  r = tested.reduce_order(price4_t(100100), 300);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(200000UL * 10000));
  BOOST_CHECK_EQUAL(actual.second, 0);
  // handler should return true
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should be decremented
  BOOST_CHECK_EQUAL(tested.count(), 0);
}

/**
 * @test Verify that the buy side of jb::itch5::order_book_cache_aware works as
 * expected.
 */
BOOST_AUTO_TEST_CASE(order_book_cache_aware_buy_range) {
  using jb::itch5::price4_t;

  std::size_t ticks = 5000;
  jb::itch5::array_based_order_book::buys_t tested(2 * ticks);

  // Check current range (min, max) ...
  auto rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(0));
  // Check is an empty side
  auto rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 0);

  // build a book around $50.00
  auto rs = tested.add_order(price4_t(100 * ticks), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.add_order(price4_t(100 * ticks + 100), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.add_order(price4_t(100 * ticks - 100), 100);
  BOOST_CHECK_EQUAL(rs, false);
  rs = tested.add_order(price4_t(100 * ticks - 200), 100);
  BOOST_CHECK_EQUAL(rs, false);
  rs = tested.add_order(price4_t(100 * ticks - 300), 100);
  BOOST_CHECK_EQUAL(rs, false);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(200 * ticks));
  // check side size
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 5);

  // change the inside 2 ticks below the limit
  rs = tested.add_order(price4_t(200 * ticks - 200), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // change the inside right below the limit
  rs = tested.add_order(price4_t(200 * ticks - 100), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 7);

  // change the inside right at the limit (therefore out)
  rs = tested.add_order(price4_t(200 * ticks), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(100 * ticks));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(300 * ticks));
  // moved 3 prices to tail
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 3);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 5);

  // change the inside far above the limit
  rs = tested.add_order(price4_t(1600 * ticks), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1500 * ticks));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(1700 * ticks));
  // moved +5 prices to tail
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 3 + 5);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 1);

  // add new price
  rs = tested.add_order(price4_t(200 * ticks + 100), 100);
  BOOST_CHECK_EQUAL(rs, false);
  // same range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1500 * ticks));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(1700 * ticks));
  // to tail
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 8 + 1);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 1);

  // remove that far above price
  // new inside is price4_t(200*ticks + 100)
  rs = tested.reduce_order(price4_t(1600 * ticks), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(100 * ticks + 100));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(300 * ticks + 100));
  // 5 prices back from the tail
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 4);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 5);

  // remove the inside
  rs = tested.reduce_order(price4_t(200 * ticks + 100), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(100 * ticks + 100));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(300 * ticks + 100));
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 4);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 4);

  // remove the inside
  rs = tested.reduce_order(price4_t(200 * ticks), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(100 * ticks + 100));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(300 * ticks + 100));
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 4);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 3);

  // remove the inside
  rs = tested.reduce_order(price4_t(200 * ticks - 100), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(100 * ticks + 100));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(300 * ticks + 100));
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 4);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 2);

  // remove the inside, range does not change...
  // new inside is right at the bottom of the range
  rs = tested.reduce_order(price4_t(200 * ticks - 200), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(100 * ticks + 100));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(300 * ticks + 100));
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 4);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 1);

  // remove the inside, range changes...
  rs = tested.reduce_order(price4_t(100 * ticks + 100), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(200 * ticks));
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 4);

  // remove the last 4 prices
  rs = tested.reduce_order(price4_t(100 * ticks), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.reduce_order(price4_t(100 * ticks - 100), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.reduce_order(price4_t(100 * ticks - 200), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.reduce_order(price4_t(100 * ticks - 300), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check  range is empty
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(0));
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 0);
}

/**
 * @test Verify that the buy side of jb::itch5::order_book_cache_aware works as
 * expected.
 */
BOOST_AUTO_TEST_CASE(order_book_cache_aware_sell_range) {
  using jb::itch5::price4_t;
  auto px_limit = jb::itch5::HIGHEST_PRICE;

  std::size_t ticks = 5000;
  jb::itch5::array_based_order_book::sells_t tested(2 * ticks);

  // Check current range (min, max) ...
  auto rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), px_limit);
  BOOST_CHECK_EQUAL(std::get<0>(rg), px_limit);
  BOOST_CHECK_EQUAL(tested.count(), 0);
  auto rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 0);

  // build a book around 10*ticks
  auto rs = tested.add_order(price4_t(1000 * ticks), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(900 * ticks));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1100 * ticks));

  rs = tested.add_order(price4_t(1000 * ticks - 100), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.add_order(price4_t(1000 * ticks + 100), 100);
  BOOST_CHECK_EQUAL(rs, false);
  rs = tested.add_order(price4_t(1000 * ticks + 200), 100);
  BOOST_CHECK_EQUAL(rs, false);
  rs = tested.add_order(price4_t(1000 * ticks + 300), 100);
  BOOST_CHECK_EQUAL(rs, false);
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 5);

  // change the inside right below the limit
  rs = tested.add_order(price4_t(900 * ticks + 100), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // still same range
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(900 * ticks));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1100 * ticks));
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 6);

  // change the inside right at the limit (P_MAX is excluded)
  rs = tested.add_order(price4_t(900 * ticks), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check current range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(800 * ticks));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1000 * ticks));
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 3);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 4);

  // change the inside
  rs = tested.add_order(price4_t(900 * ticks - 100), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(800 * ticks));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1000 * ticks));
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 3);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 5);

  // change the inside far above the limit
  rs = tested.add_order(price4_t(100 * ticks), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(200 * ticks));
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 8);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 1);

  // add a new price far below
  rs = tested.add_order(price4_t(1000 * ticks - 300), 100);
  BOOST_CHECK_EQUAL(rs, false);
  // same range
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(200 * ticks));
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 9);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 1);

  // add a new price far below
  rs = tested.add_order(price4_t(900 * ticks - 200), 100);
  BOOST_CHECK_EQUAL(rs, false);
  // same range
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(200 * ticks));
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 10);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 1);

  // remove the far above inside
  rs = tested.reduce_order(price4_t(100 * ticks), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(800 * ticks - 200));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1000 * ticks - 200));
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 5);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 5);

  // removed the inside
  rs = tested.reduce_order(price4_t(900 * ticks - 200), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(800 * ticks - 200));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1000 * ticks - 200));

  // removed the inside
  rs = tested.reduce_order(price4_t(900 * ticks - 100), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(800 * ticks - 200));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1000 * ticks - 200));

  // removed the inside
  rs = tested.reduce_order(price4_t(900 * ticks), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(800 * ticks - 200));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1000 * ticks - 200));

  // removed the inside, new inside right at the bottom of the range...
  rs = tested.reduce_order(price4_t(900 * ticks + 100), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(800 * ticks - 200));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1000 * ticks - 200));
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 5);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 1);

  // removed the inside, new range
  rs = tested.reduce_order(price4_t(1000 * ticks - 300), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(900 * ticks - 100));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1100 * ticks - 100));
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 5);

  // removed the last 5 prices
  rs = tested.reduce_order(price4_t(1000 * ticks - 100), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.reduce_order(price4_t(1000 * ticks), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.reduce_order(price4_t(1000 * ticks + 100), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.reduce_order(price4_t(1000 * ticks + 200), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.reduce_order(price4_t(1000 * ticks + 300), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), px_limit);
  BOOST_CHECK_EQUAL(std::get<0>(rg), px_limit);
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 0);
}

/**
 * @test Verify that the buy side of jb::itch5::order_book_cache_aware
 * works as expected.
 * Test suite for prices below $1.00. A smaller tick offset is used to
 * facilitate the tests
 */
BOOST_AUTO_TEST_CASE(order_book_cache_aware_buy_small_tick) {
  using jb::itch5::price4_t;

  jb::itch5::array_based_order_book::buys_t tested(3000);

  // Check current range (min, max) default values ...
  auto rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(0));

  // build a book around 15 cents
  auto rs = tested.add_order(price4_t(1500), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(3000));

  rs = tested.add_order(price4_t(1501), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.add_order(price4_t(1502), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.add_order(price4_t(1499), 100);
  BOOST_CHECK_EQUAL(rs, false);
  rs = tested.add_order(price4_t(1498), 100);
  BOOST_CHECK_EQUAL(rs, false);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(3000));

  // change the inside right below the limit
  rs = tested.add_order(price4_t(2998), 100);
  BOOST_CHECK_EQUAL(rs, true);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(3000));

  // change the inside right at the limit
  rs = tested.add_order(price4_t(2999), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check current range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(3000));

  // change the inside right above the limit
  rs = tested.add_order(price4_t(3000), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1500));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(4500));

  // change the inside far above the limit
  rs = tested.add_order(price4_t(9999), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(8499));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(159900));

  // add order far below
  rs = tested.add_order(price4_t(3001), 100);
  BOOST_CHECK_EQUAL(rs, false);

  // remove the far above inside
  rs = tested.reduce_order(price4_t(9999), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1501));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(4501));

  // change the inside far above the limit
  rs = tested.add_order(price4_t(10000), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(8500));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(160000));

  // change the inside far above the limit
  rs = tested.add_order(price4_t(10200), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(8500));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(160000));

  // remove the far above inside
  rs = tested.reduce_order(price4_t(10000), 100);
  BOOST_CHECK_EQUAL(rs, false);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(8500));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(160000));

  // remove the far above inside
  rs = tested.reduce_order(price4_t(10200), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1501));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(4501));

  // remove price to test
  rs = tested.reduce_order(price4_t(1502), 100);
  BOOST_CHECK_EQUAL(rs, false);
  rs = tested.reduce_order(price4_t(2998), 100);
  BOOST_CHECK_EQUAL(rs, false);
  // remove the inside
  rs = tested.reduce_order(price4_t(3001), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.reduce_order(price4_t(3000), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.reduce_order(price4_t(2999), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // new inside but same range
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1501));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(4501));

  // remove inside at the bottom
  rs = tested.reduce_order(price4_t(1501), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // new inside but same range
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(3000));

  // remove last 3 prices
  rs = tested.reduce_order(price4_t(1500), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.reduce_order(price4_t(1499), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.reduce_order(price4_t(1498), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(0));
}

/**
 * @test Verify that the buy side of jb::itch5::order_book_cache_aware
 * works as expected.
 * Test suite for prices below $1.00. A smaller tick offset is used to
 * facilitate the tests
 */
BOOST_AUTO_TEST_CASE(order_book_cache_aware_sell_small_tick) {
  using jb::itch5::price4_t;
  auto px_limit = jb::itch5::HIGHEST_PRICE;

  jb::itch5::array_based_order_book::sells_t tested(3000);

  // Check current range (min, max) default values ...
  auto rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), px_limit);
  BOOST_CHECK_EQUAL(std::get<0>(rg), px_limit);

  // build a book around 75 cents
  auto rs = tested.add_order(price4_t(7500), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(6000));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(9000));

  rs = tested.add_order(price4_t(7501), 100);
  BOOST_CHECK_EQUAL(rs, false);
  rs = tested.add_order(price4_t(7502), 100);
  BOOST_CHECK_EQUAL(rs, false);
  rs = tested.add_order(price4_t(7499), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.add_order(price4_t(7498), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // change the inside right at the limit
  rs = tested.add_order(price4_t(6000), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // change the inside above the limit
  rs = tested.add_order(price4_t(5999), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check current range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(4500));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(7500));

  // change the inside far above the limit
  rs = tested.add_order(price4_t(989), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(3000));

  // add order far below
  rs = tested.add_order(price4_t(5998), 100);
  BOOST_CHECK_EQUAL(rs, false);

  // remove the far above inside
  rs = tested.reduce_order(price4_t(989), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // Check new range (min, max) ...
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(4498));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(7498));

  // remove price to test
  rs = tested.reduce_order(price4_t(5999), 100);
  BOOST_CHECK_EQUAL(rs, false);
  rs = tested.reduce_order(price4_t(6000), 100);
  BOOST_CHECK_EQUAL(rs, false);

  // remove the inside
  rs = tested.reduce_order(price4_t(5998), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(4498));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(7498));

  rs = tested.reduce_order(price4_t(7498), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // new inside new range
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(5999));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(8999));

  // remove last 4 prices
  rs = tested.reduce_order(price4_t(7499), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.reduce_order(price4_t(7500), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.reduce_order(price4_t(7501), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.reduce_order(price4_t(7502), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), px_limit);
  BOOST_CHECK_EQUAL(std::get<0>(rg), px_limit);
}

/**
 * @test itch5arrayinside exception generated. Adding test case to fix the
 * problem.
 */
BOOST_AUTO_TEST_CASE(order_book_cache_aware_sell_small_tick_bug01) {
  using jb::itch5::price4_t;
  auto px_limit = jb::itch5::HIGHEST_PRICE;

  std::size_t ticks = 5000;
  jb::itch5::array_based_order_book::sells_t tested(2 * ticks);

  // Check current range (min, max) default values ...
  auto rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), px_limit);
  BOOST_CHECK_EQUAL(std::get<0>(rg), px_limit);
  auto rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 0);

  // add shares 100 @199999.9900
  auto rs = tested.add_order(price4_t(1999999900), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(2000000000 - 200 * ticks));
  BOOST_CHECK_EQUAL(std::get<0>(rg), px_limit);
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 1);

  // add 100  @0.5850
  rs = tested.add_order(price4_t(5850), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(850));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(95000));
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 1);

  // add +100 (200 now)  @0.5850
  rs = tested.add_order(price4_t(5850), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 1);
  // remove shares 100 @199999.9900
  rs = tested.reduce_order(price4_t(1999999900), 100);
  BOOST_CHECK_EQUAL(rs, false);
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 1);
  // add shares 100 @199999.9900
  rs = tested.add_order(price4_t(1999999900), 100);
  BOOST_CHECK_EQUAL(rs, false);
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 1);

  // add 100 @0.0130 (px_inside_ now)
  rs = tested.add_order(price4_t(130), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(10000));
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 2);

  // add 500 @0.0150
  rs = tested.add_order(price4_t(150), 100);
  BOOST_CHECK_EQUAL(rs, false);
  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(10000));
  rc = tested.get_counts();
  BOOST_CHECK_EQUAL(std::get<0>(rc), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rc), 3);
  // remove shares 100 @0.5850
  rs = tested.reduce_order(price4_t(5850), 100);
  BOOST_CHECK_EQUAL(rs, false);

  rg = tested.get_limits();
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(10000));
}
