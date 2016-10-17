#include <jb/itch5/order_book_depth.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::itch5::order_book_depth works as expected.
 */
BOOST_AUTO_TEST_CASE(order_book_trivial) {
  using jb::itch5::price4_t;
  jb::itch5::order_book_depth tested;

  auto actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(0));
  BOOST_CHECK_EQUAL(actual.second, 0);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(200000UL * 10000));
  BOOST_CHECK_EQUAL(actual.second, 0);
}

/**
 * @test Verity that the buy side of jb::itch5::order_book_depth works as expected.
 */
BOOST_AUTO_TEST_CASE(order_book_buy) {
  using jb::itch5::price4_t;
  jb::itch5::order_book_depth tested;

  jb::itch5::buy_sell_indicator_t const BUY(u'B');
  jb::itch5::buy_sell_indicator_t const SELL(u'S');

  // Add a new order ...
  tested.handle_add_order(BUY, price4_t(100000), 100);
  // ... the offer should not change ...
  auto actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(200000UL * 10000));
  BOOST_CHECK_EQUAL(actual.second, 0);

  // .. but the bid should ...
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);

  // ... adding below the best bid has no effect ...
  tested.handle_add_order(BUY, price4_t(99900), 300);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);
  // ... update at the bid increases the qty ...
  tested.handle_add_order(BUY, price4_t(100000), 400);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 500);
  // ... a better price changes both price and qty ...
  tested.handle_add_order(BUY, price4_t(100100), 200);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 200);
  // ... decrease below the bid has no effect ...
  tested.handle_order_reduced(BUY, price4_t(100000), 400);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 200);
  // ... even when it is over the existing quantity ...
  tested.handle_order_reduced(BUY, price4_t(100000), 200);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 200);
  // ... deleting the best bid uncovers the best price ...
  tested.handle_order_reduced(BUY, price4_t(100100), 200);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 300);
}

/**
 * @test Verity that the sell side of jb::itch5::order_book_depth works as expected.
 */
BOOST_AUTO_TEST_CASE(order_book_sell) {
  using jb::itch5::price4_t;
  jb::itch5::order_book_depth tested;

  jb::itch5::buy_sell_indicator_t const BUY(u'B');
  jb::itch5::buy_sell_indicator_t const SELL(u'S');

  // Add a new order ...
  tested.handle_add_order(SELL, price4_t(100000), 100);
  // ... the offer should not change ...
  auto actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(0));
  BOOST_CHECK_EQUAL(actual.second, 0);

  // .. but the offer should ...
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);

  // ... adding above the best offer has no effect ...
  tested.handle_add_order(SELL, price4_t(100100), 300);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);
  // ... update at the offer increases the qty ...
  tested.handle_add_order(SELL, price4_t(100000), 400);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 500);
  // ... a better price changes both price and qty ...
  tested.handle_add_order(SELL, price4_t(99900), 200);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 200);
  // ... decrease above the offer has no effect ...
  tested.handle_order_reduced(SELL, price4_t(100000), 400);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 200);
  // ... even when it is over the existing quantity ...
  tested.handle_order_reduced(SELL, price4_t(100000), 200);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 200);
  // ... deleting the best offer uncovers the best price ...
  tested.handle_order_reduced(SELL, price4_t(99900), 200);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 300);
}
