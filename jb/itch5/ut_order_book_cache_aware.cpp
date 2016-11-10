#include <jb/itch5/order_book_cache_aware.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::itch5::order_book_cache_aware works as expected.
 */
BOOST_AUTO_TEST_CASE(order_book_cache_aware_trivial) {
  using jb::itch5::price4_t;
  jb::itch5::order_book_cache_aware tested;

  auto thetick = tested.tick_offset();
  BOOST_CHECK_EQUAL(thetick, 5000); // default value

  tested.tick_offset(1000);
  thetick = tested.tick_offset();
  BOOST_CHECK_EQUAL(thetick, 1000);

  BOOST_CHECK_THROW(tested.tick_offset(-1), std::exception);
  thetick = tested.tick_offset();
  BOOST_CHECK_EQUAL(thetick, 1000); // previous value

  tested.tick_offset(5000); // back to default to run tests
  thetick = tested.tick_offset();
  BOOST_CHECK_EQUAL(thetick, 5000);

  // BOOST_CHECK_EQUAL(jb::itch5::order_book_cache_aware::tick_offset_, 0);
  auto actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(0));
  BOOST_CHECK_EQUAL(actual.second, 0);
  BOOST_CHECK_EQUAL(tested.best_bid_price(), price4_t(0));
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(200000UL * 10000));
  BOOST_CHECK_EQUAL(actual.second, 0);
  BOOST_CHECK_EQUAL(tested.best_offer_price(), price4_t(200000UL * 10000));
  //  book_depth should be 0
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 0);
}

/**
 * @test Verify that the buy side of jb::itch5::order_book_cache_aware works as
 * expected.
 */
BOOST_AUTO_TEST_CASE(order_book_cache_aware_buy) {
  using jb::itch5::price4_t;
  jb::itch5::order_book_cache_aware tested;
  jb::itch5::buy_sell_indicator_t const BUY(u'B');
  jb::itch5::buy_sell_indicator_t const SELL(u'S');

  auto ticks = tested.tick_offset();
  BOOST_CHECK_EQUAL(ticks, 5000); // default value

  // Add a new order ...
  auto r = tested.handle_add_order(BUY, price4_t(100000), 100);
  // ... the offer should not change ...
  auto actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(200000UL * 10000));
  BOOST_CHECK_EQUAL(actual.second, 0);
  // .. but the bid should ...
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);
  BOOST_CHECK_EQUAL(tested.best_bid_price(), price4_t(100000));
  // handler should return 0... it is the first price set
  BOOST_CHECK_EQUAL(std::get<0>(r), 0);
  BOOST_CHECK_EQUAL(std::get<1>(r), 0);
  // check the range
  auto rg = tested.price_range(BUY);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(600000));
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 1);

  // ... adding below the best bid has no effect ...
  r = tested.handle_add_order(BUY, price4_t(99900), 300);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);
  BOOST_CHECK_EQUAL(std::get<0>(r), 0);
  BOOST_CHECK_EQUAL(std::get<1>(r), 0);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 2);

  // ... update at the bid increases the qty ...
  r = tested.handle_add_order(BUY, price4_t(100000), 400);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 500);
  BOOST_CHECK_EQUAL(std::get<0>(r), 0);
  BOOST_CHECK_EQUAL(std::get<1>(r), 0);
  // .. and the book_depth should not be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 2);

  // ... a better price changes both price (+1 ticks) and qty ...
  r = tested.handle_add_order(BUY, price4_t(100100), 200);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 200);
  BOOST_CHECK_EQUAL(std::get<0>(r), 1); // inside moves one tick up
  BOOST_CHECK_EQUAL(std::get<1>(r), 0);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 3);

  // ... decrease below the bid has no effect ...
  r = tested.handle_order_reduced(BUY, price4_t(100000), 400);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 200);
  BOOST_CHECK_EQUAL(tested.best_bid_price(), price4_t(100100));
  BOOST_CHECK_EQUAL(std::get<0>(r), 0);
  BOOST_CHECK_EQUAL(std::get<1>(r), 0);
  // .. and the book depth should not be decremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 3);

  // ... even when it is over the existing quantity ...
  r = tested.handle_order_reduced(BUY, price4_t(100000), 200);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 200);
  BOOST_CHECK_EQUAL(std::get<0>(r), 0);
  BOOST_CHECK_EQUAL(std::get<1>(r), 0);
  // .. and the book_depth should be decremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 2);

  // ... deleting the best bid uncovers the best price ...
  r = tested.handle_order_reduced(BUY, price4_t(100100), 200);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 300);
  BOOST_CHECK_EQUAL(tested.best_bid_price(), price4_t(99900));
  BOOST_CHECK_EQUAL(std::get<0>(r), 2);
  BOOST_CHECK_EQUAL(std::get<1>(r), 0);
  // .. and the book_depth should be decremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 1);

  // ... deleting the remaining price takes the book depth to 0
  r = tested.handle_order_reduced(BUY, price4_t(99900), 300);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(0));
  BOOST_CHECK_EQUAL(actual.second, 0);
  BOOST_CHECK_EQUAL(tested.best_bid_price(), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<0>(r), 2 * ticks);
  BOOST_CHECK_EQUAL(std::get<1>(r), 0);
  // .. and the book_depth should be decremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 0);
}

/**
 * @test Verity that the sell side of jb::itch5::order_book_cache_aware works as
 * expected.
 */
BOOST_AUTO_TEST_CASE(order_book_cache_aware_sell) {
  using jb::itch5::price4_t;
  jb::itch5::order_book_cache_aware tested;

  jb::itch5::buy_sell_indicator_t const BUY(u'B');
  jb::itch5::buy_sell_indicator_t const SELL(u'S');

  auto ticks = tested.tick_offset();
  BOOST_CHECK_EQUAL(ticks, 5000); // default value

  // Add a new order ...
  auto r = tested.handle_add_order(SELL, price4_t(100000), 100);
  // ... the offer should not change ...
  auto actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(0));
  BOOST_CHECK_EQUAL(actual.second, 0);
  // .. but the offer should ...
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);
  BOOST_CHECK_EQUAL(tested.best_offer_price(), price4_t(100000));
  BOOST_CHECK_EQUAL(std::get<0>(r), 0); // first order
  BOOST_CHECK_EQUAL(std::get<1>(r), 0);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 1);

  // ... adding above the best offer has no effect ...
  r = tested.handle_add_order(SELL, price4_t(100100), 300);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);
  BOOST_CHECK_EQUAL(std::get<0>(r), 0);
  BOOST_CHECK_EQUAL(std::get<1>(r), 0);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 2);

  // ... update at the offer increases the qty ...
  r = tested.handle_add_order(SELL, price4_t(100000), 400);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 500);
  // handler should return true... it is an inside change
  BOOST_CHECK_EQUAL(std::get<0>(r), 0);
  BOOST_CHECK_EQUAL(std::get<1>(r), 0);
  // .. and the book_depth should not change
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 2);

  // ... a better price changes both price and qty ...
  r = tested.handle_add_order(SELL, price4_t(99900), 200);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 200);
  BOOST_CHECK_EQUAL(tested.best_offer_price(), price4_t(99900));
  BOOST_CHECK_EQUAL(std::get<0>(r), 1);
  BOOST_CHECK_EQUAL(std::get<1>(r), 0);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 3);

  // ... decrease above the offer has no effect ...
  r = tested.handle_order_reduced(SELL, price4_t(100000), 400);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 200);
  BOOST_CHECK_EQUAL(std::get<0>(r), 0);
  BOOST_CHECK_EQUAL(std::get<1>(r), 0);
  // .. and the book_depth should not change
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 3);

  // ... even when it is over the existing quantity ...
  r = tested.handle_order_reduced(SELL, price4_t(100000), 200);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 200);
  BOOST_CHECK_EQUAL(std::get<0>(r), 0);
  BOOST_CHECK_EQUAL(std::get<1>(r), 0);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 2);

  // ... deleting the best offer uncovers the best price ...
  r = tested.handle_order_reduced(SELL, price4_t(99900), 200);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 300);
  BOOST_CHECK_EQUAL(tested.best_offer_price(), price4_t(100100));
  BOOST_CHECK_EQUAL(std::get<0>(r), 2);
  BOOST_CHECK_EQUAL(std::get<1>(r), 0);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 1);

  // ... deleting the remaining price takes the book depth to 0
  r = tested.handle_order_reduced(SELL, price4_t(100100), 300);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(200000UL * 10000));
  BOOST_CHECK_EQUAL(actual.second, 0);
  BOOST_CHECK_EQUAL(tested.best_offer_price(), price4_t(200000UL * 10000));
  // handler should return true
  BOOST_CHECK_EQUAL(std::get<0>(r), 2 * ticks);
  BOOST_CHECK_EQUAL(std::get<1>(r), 0);
  // .. and the book_depth should be decremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 0);
}

/**
 * @test Verify that the buy side of jb::itch5::order_book_cache_aware handles
 * errors as expected.
 */
BOOST_AUTO_TEST_CASE(order_book_cache_aware_buy_errors) {
  using jb::itch5::price4_t;
  jb::itch5::order_book_cache_aware tested;

  jb::itch5::buy_sell_indicator_t const BUY(u'B');

  // Add two orders to the book ...
  (void)tested.handle_add_order(BUY, price4_t(100000), 100);
  (void)tested.handle_add_order(BUY, price4_t(110000), 200);

  // ... check the best bid ...
  auto actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(110000));
  BOOST_CHECK_EQUAL(actual.second, 200);

  // ... remove the first order, once should work, the second time
  // should fail ...
  (void)tested.handle_order_reduced(BUY, price4_t(100000), 100);
  BOOST_CHECK_THROW(
      tested.handle_order_reduced(BUY, price4_t(100000), 100), std::exception);

  // ... check the best bid ...
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(110000));
  BOOST_CHECK_EQUAL(actual.second, 200);
}

/**
 * @test Verify that the sell side of jb::itch5::order_book_cache_aware handles
 * errors as expected.
 */
BOOST_AUTO_TEST_CASE(order_book_cache_aware_sell_errors) {
  using jb::itch5::price4_t;
  jb::itch5::order_book_cache_aware tested;

  jb::itch5::buy_sell_indicator_t const SELL(u'S');

  // Add two orders to the book ...
  (void)tested.handle_add_order(SELL, price4_t(120000), 100);
  (void)tested.handle_add_order(SELL, price4_t(110000), 200);

  // ... check the best offer ...
  auto actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(110000));
  BOOST_CHECK_EQUAL(actual.second, 200);

  // ... remove the first order, once should work, the second time
  // should fail ...
  (void)tested.handle_order_reduced(SELL, price4_t(120000), 100);
  BOOST_CHECK_THROW(
      tested.handle_order_reduced(SELL, price4_t(120000), 100), std::exception);

  // ... check the best offer ...
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(110000));
  BOOST_CHECK_EQUAL(actual.second, 200);
}

/**
 * @test Verify that the buy side of jb::itch5::order_book_cache_aware works as
 * expected.
 */
BOOST_AUTO_TEST_CASE(order_book_cache_aware_buy_range) {
  using jb::itch5::price4_t;
  jb::itch5::order_book_cache_aware tested;

  jb::itch5::buy_sell_indicator_t const BUY(u'B');
  jb::itch5::buy_sell_indicator_t const SELL(u'S');

  auto ticks = tested.tick_offset();
  BOOST_CHECK_EQUAL(ticks, 5000); // default value

  // Check current range (min, max) ...
  auto rg = tested.price_range(BUY);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(200 * ticks));

  // build a book around ticks
  auto rs = tested.handle_add_order(BUY, price4_t(100 * ticks), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_add_order(BUY, price4_t(100 * ticks + 100), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_add_order(BUY, price4_t(100 * ticks - 100), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_add_order(BUY, price4_t(100 * ticks - 200), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_add_order(BUY, price4_t(100 * ticks - 300), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);

  // change the inside 2 ticks below the limit
  rs = tested.handle_add_order(BUY, price4_t(200 * ticks - 200), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), ticks - 3);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);

  // change the inside right below the limit
  rs = tested.handle_add_order(BUY, price4_t(200 * ticks - 100), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  // Check current range (min, max) ...
  rg = tested.price_range(BUY);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(200 * ticks));

  // change the inside right at the limit (therefore out)
  rs = tested.handle_add_order(BUY, price4_t(200 * ticks), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 3);
  // Check new range (min, max) ...
  rg = tested.price_range(BUY);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(100 * ticks));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(300 * ticks));

  // change the inside far above the limit
  rs = tested.handle_add_order(BUY, price4_t(1600 * ticks), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 14 * ticks);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 5);
  // Check new range (min, max) ...
  rg = tested.price_range(BUY);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1500 * ticks));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(1700 * ticks));

  // add new price far below the limit
  rs = tested.handle_add_order(BUY, price4_t(200 * ticks + 100), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  // same range (min, max) ...
  rg = tested.price_range(BUY);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1500 * ticks));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(1700 * ticks));

  // remove that far above new price
  rs = tested.handle_order_reduced(BUY, price4_t(1600 * ticks), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 14 * ticks - 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 4);
  // Check new range (min, max) ...
  rg = tested.price_range(BUY);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(100 * ticks + 100));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(300 * ticks + 100));

  // remove the inside
  rs = tested.handle_order_reduced(BUY, price4_t(200 * ticks + 100), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  // Check new range (min, max) ...
  rg = tested.price_range(BUY);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(100 * ticks + 100));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(300 * ticks + 100));

  // remove the inside
  rs = tested.handle_order_reduced(BUY, price4_t(200 * ticks), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  // Check new range (min, max) ...
  rg = tested.price_range(BUY);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(100 * ticks + 100));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(300 * ticks + 100));

  // remove the inside
  rs = tested.handle_order_reduced(BUY, price4_t(200 * ticks - 100), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  // Check new range (min, max) ...
  rg = tested.price_range(BUY);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(100 * ticks + 100));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(300 * ticks + 100));

  // remove the inside, range does not change...
  // new inside is right at the bottom of the range
  rs = tested.handle_order_reduced(BUY, price4_t(200 * ticks - 200), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), ticks - 3);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  // Check new range (min, max) ...
  rg = tested.price_range(BUY);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(100 * ticks + 100));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(300 * ticks + 100));

  // remove the inside, range changes...
  rs = tested.handle_order_reduced(BUY, price4_t(100 * ticks + 100), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 4);
  // Check new range (min, max) ...
  rg = tested.price_range(BUY);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(200 * ticks));

  // remove the last 4 prices
  rs = tested.handle_order_reduced(BUY, price4_t(100 * ticks), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_order_reduced(BUY, price4_t(100 * ticks - 100), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_order_reduced(BUY, price4_t(100 * ticks - 200), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_order_reduced(BUY, price4_t(100 * ticks - 300), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 2 * ticks); // max change
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  // Check same range (min, max) ...
  rg = tested.price_range(BUY);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(200 * ticks));
}

/**
 * @test Verify that the buy side of jb::itch5::order_book_cache_aware works as
 * expected.
 */
BOOST_AUTO_TEST_CASE(order_book_cache_aware_sell_range) {
  using jb::itch5::price4_t;

  jb::itch5::buy_sell_indicator_t const BUY(u'B');
  jb::itch5::buy_sell_indicator_t const SELL(u'S');

  jb::itch5::order_book_cache_aware tested;
  auto ticks = tested.tick_offset();
  BOOST_CHECK_EQUAL(ticks, 5000); // default value

  // Check current range (min, max) ...
  auto rg = tested.price_range(SELL);
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(200 * ticks));

  // build a book around 10*ticks
  auto rs = tested.handle_add_order(SELL, price4_t(1000 * ticks), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  // Check new range (min, max) ...
  rg = tested.price_range(SELL);
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(900 * ticks));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1100 * ticks));

  rs = tested.handle_add_order(SELL, price4_t(1000 * ticks - 100), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_add_order(SELL, price4_t(1000 * ticks + 100), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_add_order(SELL, price4_t(1000 * ticks + 200), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_add_order(SELL, price4_t(1000 * ticks + 300), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);

  // change the inside 1 tick above the limit
  rs = tested.handle_add_order(SELL, price4_t(900 * ticks + 100), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), ticks - 2);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  // still same range
  rg = tested.price_range(SELL);
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(900 * ticks));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1100 * ticks));

  // change the inside right at the limit
  rs = tested.handle_add_order(SELL, price4_t(900 * ticks), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  // Check current range (min, max) ...
  rg = tested.price_range(SELL);
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(900 * ticks));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1100 * ticks));

  // change the inside right below the limit
  rs = tested.handle_add_order(SELL, price4_t(900 * ticks - 100), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 5);
  // Check new range (min, max) ...
  rg = tested.price_range(SELL);
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(800 * ticks - 100));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1000 * ticks - 100));

  // change the inside far above the limit
  rs = tested.handle_add_order(SELL, price4_t(100 * ticks), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 8 * ticks - 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 3);
  // Check new range (min, max) ...
  rg = tested.price_range(SELL);
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(200 * ticks));

  // add a new price far below
  rs = tested.handle_add_order(SELL, price4_t(1000 * ticks - 300), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  // same range
  rg = tested.price_range(SELL);
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(200 * ticks));

  // add a new price far below
  rs = tested.handle_add_order(SELL, price4_t(900 * ticks - 200), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  // same range
  rg = tested.price_range(SELL);
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(200 * ticks));

  // remove the far above inside
  rs = tested.handle_order_reduced(SELL, price4_t(100 * ticks), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 8 * ticks - 2);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 5);
  // Check new range (min, max) ...
  rg = tested.price_range(SELL);
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(800 * ticks - 200));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1000 * ticks - 200));

  // removed the inside
  rs = tested.handle_order_reduced(SELL, price4_t(900 * ticks - 200), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  // Check new range (min, max) ...
  rg = tested.price_range(SELL);
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(800 * ticks - 200));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1000 * ticks - 200));

  // removed the inside
  rs = tested.handle_order_reduced(SELL, price4_t(900 * ticks - 100), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  // Check new range (min, max) ...
  rg = tested.price_range(SELL);
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(800 * ticks - 200));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1000 * ticks - 200));

  // removed the inside
  rs = tested.handle_order_reduced(SELL, price4_t(900 * ticks), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  // Check new range (min, max) ...
  rg = tested.price_range(SELL);
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(800 * ticks - 200));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1000 * ticks - 200));

  // removed the inside, new inside right at the bottom of the range...
  rs = tested.handle_order_reduced(SELL, price4_t(900 * ticks + 100), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), ticks - 4);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  // Check new range (min, max) ...
  rg = tested.price_range(SELL);
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(800 * ticks - 200));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1000 * ticks - 200));

  // removed the inside, new range
  rs = tested.handle_order_reduced(SELL, price4_t(1000 * ticks - 300), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 2);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 5);
  // Check new range (min, max) ...
  rg = tested.price_range(SELL);
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(900 * ticks - 100));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1100 * ticks - 100));

  // removed the last 5 prices
  rs = tested.handle_order_reduced(SELL, price4_t(1000 * ticks - 100), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_order_reduced(SELL, price4_t(1000 * ticks), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_order_reduced(SELL, price4_t(1000 * ticks + 100), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_order_reduced(SELL, price4_t(1000 * ticks + 200), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_order_reduced(SELL, price4_t(1000 * ticks + 300), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 2 * ticks); // max change
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(900 * ticks - 100));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1100 * ticks - 100));
}

/**
 * @test Verify that the buy side of jb::itch5::order_book_cache_aware
 * works as expected.
 * Test suite for prices below $1.00. A smaller tick offset is used to
 * facilitate the tests
 */
BOOST_AUTO_TEST_CASE(order_book_cache_aware_buy_small_tick) {
  using jb::itch5::price4_t;
  jb::itch5::order_book_cache_aware tested;

  jb::itch5::buy_sell_indicator_t const BUY(u'B');
  jb::itch5::buy_sell_indicator_t const SELL(u'S');

  auto default_ticks = tested.tick_offset();
  BOOST_CHECK_EQUAL(default_ticks, 5000); // default value
  tested.tick_offset(1500);               // 0 .. 30 cents
  auto ticks = tested.tick_offset();
  BOOST_CHECK_EQUAL(ticks, 1500);

  // Check current range (min, max) default values ...
  auto rg = tested.price_range(BUY);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(200 * default_ticks));

  // build a book around 15 cents
  auto rs = tested.handle_add_order(BUY, price4_t(1500), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rg = tested.price_range(BUY);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(3000));

  rs = tested.handle_add_order(BUY, price4_t(1501), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_add_order(BUY, price4_t(1502), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_add_order(BUY, price4_t(1499), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_add_order(BUY, price4_t(1498), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);

  // change the inside right below the limit
  rs = tested.handle_add_order(BUY, price4_t(2998), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1496);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);

  // change the inside right at the limit
  rs = tested.handle_add_order(BUY, price4_t(2999), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  // Check current range (min, max) ...
  rg = tested.price_range(BUY);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(3000));

  // change the inside right above the limit
  rs = tested.handle_add_order(BUY, price4_t(3000), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 2);
  // Check new range (min, max) ...
  rg = tested.price_range(BUY);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1500));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(4500));

  // change the inside far above the limit
  rs = tested.handle_add_order(BUY, price4_t(9999), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 6999);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 6);
  // Check new range (min, max) ...
  rg = tested.price_range(BUY);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(8499));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(159900));

  // add order far below
  rs = tested.handle_add_order(BUY, price4_t(3001), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);

  // remove the far above inside
  rs = tested.handle_order_reduced(BUY, price4_t(9999), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 6998);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 6);
  // Check new range (min, max) ...
  rg = tested.price_range(BUY);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1501));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(4501));

  // change the inside far above the limit
  rs = tested.handle_add_order(BUY, price4_t(10000), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 6999);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 6);
  // Check new range (min, max) ...
  rg = tested.price_range(BUY);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(8500));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(160000));

  // remove the far above inside
  rs = tested.handle_order_reduced(BUY, price4_t(10000), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 6999);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 6);
  // Check new range (min, max) ...
  rg = tested.price_range(BUY);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1501));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(4501));

  // remove price to test
  rs = tested.handle_order_reduced(BUY, price4_t(1502), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_order_reduced(BUY, price4_t(2998), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  // remove the inside
  rs = tested.handle_order_reduced(BUY, price4_t(3001), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_order_reduced(BUY, price4_t(3000), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_order_reduced(BUY, price4_t(2999), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1498);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  // new inside but same range
  rg = tested.price_range(BUY);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(1501));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(4501));

  // remove inside at the bottom
  rs = tested.handle_order_reduced(BUY, price4_t(1501), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 3);
  // new inside but same range
  rg = tested.price_range(BUY);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(3000));

  // remove last 3 prices
  rs = tested.handle_order_reduced(BUY, price4_t(1500), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_order_reduced(BUY, price4_t(1499), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_order_reduced(BUY, price4_t(1498), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 2 * ticks);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rg = tested.price_range(BUY);
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(3000));
}

/**
 * @test Verify that the buy side of jb::itch5::order_book_cache_aware
 * works as expected.
 * Test suite for prices below $1.00. A smaller tick offset is used to
 * facilitate the tests
 */
BOOST_AUTO_TEST_CASE(order_book_cache_aware_sell_small_tick) {
  using jb::itch5::price4_t;
  jb::itch5::order_book_cache_aware tested;

  jb::itch5::buy_sell_indicator_t const BUY(u'B');
  jb::itch5::buy_sell_indicator_t const SELL(u'S');

  tested.tick_offset(1500); // 0 .. 30 cents
  auto ticks = tested.tick_offset();
  BOOST_CHECK_EQUAL(ticks, 1500);

  // Check current range (min, max) default values ...
  auto rg = tested.price_range(SELL);
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(200 * ticks));

  // build a book around 75 cents
  auto rs = tested.handle_add_order(SELL, price4_t(7500), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rg = tested.price_range(SELL);
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(6000));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(9000));

  rs = tested.handle_add_order(SELL, price4_t(7501), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_add_order(SELL, price4_t(7502), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_add_order(SELL, price4_t(7499), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_add_order(SELL, price4_t(7498), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);

  // change the inside right at the limit
  rs = tested.handle_add_order(SELL, price4_t(6000), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1498);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);

  // change the inside above the limit
  rs = tested.handle_add_order(SELL, price4_t(5999), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 4);
  // Check current range (min, max) ...
  rg = tested.price_range(SELL);
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(4499));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(7499));

  // change the inside far above the limit
  rs = tested.handle_add_order(SELL, price4_t(999), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 5000);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 3);
  // Check new range (min, max) ...
  rg = tested.price_range(SELL);
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(3000));

  // add order far below
  rs = tested.handle_add_order(SELL, price4_t(5998), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);

  // remove the far above inside
  rs = tested.handle_order_reduced(SELL, price4_t(999), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 4999);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 4);
  // Check new range (min, max) ...
  rg = tested.price_range(SELL);
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(4498));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(7498));

  // remove price to test
  rs = tested.handle_order_reduced(SELL, price4_t(5999), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_order_reduced(SELL, price4_t(6000), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 0);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);

  // remove the inside
  rs = tested.handle_order_reduced(SELL, price4_t(5998), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1500);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 4);
  rg = tested.price_range(SELL);
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(5998));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(8998));

  rs = tested.handle_order_reduced(SELL, price4_t(7498), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  // new inside new range
  rg = tested.price_range(SELL);
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(5998));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(8998));

  // remove last 4 prices
  rs = tested.handle_order_reduced(SELL, price4_t(7499), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_order_reduced(SELL, price4_t(7500), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_order_reduced(SELL, price4_t(7501), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 1);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rs = tested.handle_order_reduced(SELL, price4_t(7502), 100);
  BOOST_CHECK_EQUAL(std::get<0>(rs), 2 * ticks);
  BOOST_CHECK_EQUAL(std::get<1>(rs), 0);
  rg = tested.price_range(SELL);
  BOOST_CHECK_EQUAL(std::get<1>(rg), price4_t(0));
  BOOST_CHECK_EQUAL(std::get<0>(rg), price4_t(3000));
}
