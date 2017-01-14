#include <jb/itch5/array_based_order_book.hpp>
#include <jb/itch5/testing/ut_type_based_order_book.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Trivial verification that array_based_order_book works as expected.
 */
BOOST_AUTO_TEST_CASE(array_based_order_book_trivial) {
  using namespace jb::itch5;
  testing::test_order_book_type_trivial<array_based_order_book>();
}

/**
 * @test Verify that array_based_order_book handles add and reduce as expected.
 */

BOOST_AUTO_TEST_CASE(array_based_order_book_test) {
  using namespace jb::itch5;
  testing::test_order_book_type_add_reduce<array_based_order_book>();
}

/**
 * @test Verify that array_based_order_book handles errors as expected.
 */
BOOST_AUTO_TEST_CASE(array_based_order_book_errors) {
  using namespace jb::itch5;
  testing::test_order_book_type_errors<array_based_order_book>();
  testing::test_order_book_type_errors_spec<array_based_order_book>();
}

/**
 * @test Verify that the buy side of array_based_order_book works as
 * expected.
 * Test inside changes at the top levels, and one price moved from
 * bottom to top to become the new inside.
 */
BOOST_AUTO_TEST_CASE(array_based_order_book_buy) {
  using namespace jb::itch5;

  const std::size_t ticks = 5000;
  array_based_order_book::buys_t tested(
      array_based_order_book::config().max_size(2 * ticks));

  // Add a new order ...
  auto r = tested.add_order(price4_t(100000), 100);
  // .. best quote should change
  auto actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);
  // handler should return true... it is the first price set
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.count(), 1);

  // ... adding below the best bid has no effect ...
  r = tested.add_order(price4_t(99900), 300);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);
  BOOST_CHECK_EQUAL(r, false);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.count(), 2);

  // ... adding below the top_level_ low limit has not effect...
  r = tested.add_order(price4_t(500), 700);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);
  BOOST_CHECK_EQUAL(r, false);
  // worst bid is now at the bottom_levels
  actual = tested.worst_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(500));
  BOOST_CHECK_EQUAL(actual.second, 700);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.count(), 3);

  // ... update at the bid increases the qty ...
  r = tested.add_order(price4_t(100000), 400);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 500);
  // ... still change at the bid
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should not be incremented
  BOOST_CHECK_EQUAL(tested.count(), 3);

  // ... a better price changes both price and qty ...
  r = tested.add_order(price4_t(100100), 200);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 200);
  BOOST_CHECK_EQUAL(r, true); // inside moves up
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.count(), 4);

  // ... decrease (400 out of 500) below the bid has no effect ...
  r = tested.reduce_order(price4_t(100000), 400);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 200);
  BOOST_CHECK_EQUAL(r, false);
  // .. and the book depth should not be decremented
  BOOST_CHECK_EQUAL(tested.count(), 4);

  // ... even when it is over the existing quantity ...
  // reducing 200 out of 100, count should decrement ...
  r = tested.reduce_order(price4_t(100000), 200);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 200);
  BOOST_CHECK_EQUAL(r, false);
  // .. and the book_depth should be decremented
  BOOST_CHECK_EQUAL(tested.count(), 3);

  // ... deleting the best bid uncovers the best price ...
  r = tested.reduce_order(price4_t(100100), 200);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 300);
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should be decremented
  BOOST_CHECK_EQUAL(tested.count(), 2);

  // ... deleting the remaining price takes the book depth to 0
  r = tested.reduce_order(price4_t(99900), 300);
  actual = tested.best_quote();
  // ... it moves one price from the bottom to the top
  // and it is the new inside now...
  BOOST_CHECK_EQUAL(actual.first, price4_t(500));
  BOOST_CHECK_EQUAL(actual.second, 700);
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should be decremented
  BOOST_CHECK_EQUAL(tested.count(), 1);
  r = tested.reduce_order(price4_t(500), 700);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(0));
  BOOST_CHECK_EQUAL(actual.second, 0);
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should be decremented
  BOOST_CHECK_EQUAL(tested.count(), 0);
}

/**
 * @test Verity that the sell side of array_based_order_book works as
 * expected.
 * Test inside changes at the top levels, and one price moved from
 * bottom to top to become the new inside.
 */
BOOST_AUTO_TEST_CASE(array_based_order_book_sell) {
  using namespace jb::itch5;

  const std::size_t ticks = 5000;
  array_based_order_book::sells_t tested(
      array_based_order_book::config().max_size(2 * ticks));

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

  // ... adding above the top_level_ low limit has not effect...
  // but count is incremented...
  r = tested.add_order(price4_t(1000000), 100);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);
  BOOST_CHECK_EQUAL(r, false);
  // worst bid is now at the bottom_levels
  actual = tested.worst_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(1000000));
  BOOST_CHECK_EQUAL(actual.second, 100);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.count(), 3);

  // ... update at the offer increases the qty ...
  r = tested.add_order(price4_t(100000), 400);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 500);
  // handler should return true... it is an inside change
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should not change
  BOOST_CHECK_EQUAL(tested.count(), 3);

  // ... a better price changes both price and qty ...
  r = tested.add_order(price4_t(99900), 200);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 200);
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.count(), 4);

  // ... decrease above the offer has no effect ...
  // reduce 400 out of 500, so still same count
  r = tested.reduce_order(price4_t(100000), 400);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 200);
  BOOST_CHECK_EQUAL(r, false);
  // .. and the book_depth should not change
  BOOST_CHECK_EQUAL(tested.count(), 4);

  // ... even when it is over the existing quantity ...
  // but reduce the count
  r = tested.reduce_order(price4_t(100000), 200);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 200);
  BOOST_CHECK_EQUAL(r, false);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.count(), 3);

  // ... deleting the best offer uncovers the best price ...
  r = tested.reduce_order(price4_t(99900), 200);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 300);
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.count(), 2);

  // ... deleting the remaining price takes the book depth to 0
  r = tested.reduce_order(price4_t(100100), 300);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(1000000));
  BOOST_CHECK_EQUAL(actual.second, 100);
  // handler should return true
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should be decremented
  BOOST_CHECK_EQUAL(tested.count(), 1);

  // ... deleting the remaining price takes the book depth to 0
  r = tested.reduce_order(price4_t(1000000), 100);
  actual = tested.best_quote();
  BOOST_CHECK_EQUAL(actual.first, price4_t(200000UL * 10000));
  BOOST_CHECK_EQUAL(actual.second, 0);
  // handler should return true
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should be decremented
  BOOST_CHECK_EQUAL(tested.count(), 0);
}

/**
 * @test Verify that the buy side of array_based_order_book works as
 * expected.
 * Works adding and removing prices at the limit of top level range
 * to verify move form and to the bottom level work as expected on those
 * border cases.
 */
BOOST_AUTO_TEST_CASE(array_based_order_book_buy_range) {
  using namespace jb::itch5;

  const std::size_t ticks = 5000;
  array_based_order_book::buys_t tested(
      array_based_order_book::config().max_size(2 * ticks));

  // build a book around $50.00 (limits built around when
  // first price is added...
  // top end limit is now 200 * ticks...
  auto rs = tested.add_order(price4_t(100 * ticks), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // now add one better price, inside changes...
  rs = tested.add_order(price4_t(100 * ticks + 100), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // and 3 prices(*) down, nothing changes, but the count..
  rs = tested.add_order(price4_t(100 * ticks - 100), 100);
  BOOST_CHECK_EQUAL(rs, false);
  rs = tested.add_order(price4_t(100 * ticks - 200), 100);
  BOOST_CHECK_EQUAL(rs, false);
  rs = tested.add_order(price4_t(100 * ticks - 300), 100);
  BOOST_CHECK_EQUAL(rs, false);
  BOOST_CHECK_EQUAL(tested.count(), 5);

  // change the inside 2 ticks below the limit
  rs = tested.add_order(price4_t(200 * ticks - 200), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // change the inside right below the limit
  rs = tested.add_order(price4_t(200 * ticks - 100), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // change the inside right at the limit (therefore out)
  // new limits are now 100*tick and 300*tick
  // the 3 prices (*) are now at the bottom levels (test move_to_bottom)
  rs = tested.add_order(price4_t(200 * ticks), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // change the inside far above the limit
  // all the prices, but the new inside, are now at the bottom levels
  // this is in preration to test move_from_botom...
  rs = tested.add_order(price4_t(1600 * ticks), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // add new price, this is the second best price now....
  rs = tested.add_order(price4_t(200 * ticks + 100), 100);
  BOOST_CHECK_EQUAL(rs, false);

  // remove that far above price
  // new inside is price4_t(200*ticks + 100, previous one)...
  // prices were moved_from_bottom, some were not.,,
  rs = tested.reduce_order(price4_t(1600 * ticks), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // now remove prices around 200*ticks~... should not move prices...
  // remove the inside
  rs = tested.reduce_order(price4_t(200 * ticks + 100), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // remove the inside
  rs = tested.reduce_order(price4_t(200 * ticks), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // remove the inside
  rs = tested.reduce_order(price4_t(200 * ticks - 100), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // new inside is right at the bottom of the range...
  rs = tested.reduce_order(price4_t(200 * ticks - 200), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // remove the inside, prices are move from the bottom
  rs = tested.reduce_order(price4_t(100 * ticks + 100), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // remove the last 4 prices
  rs = tested.reduce_order(price4_t(100 * ticks), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.reduce_order(price4_t(100 * ticks - 100), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.reduce_order(price4_t(100 * ticks - 200), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.reduce_order(price4_t(100 * ticks - 300), 100);
  BOOST_CHECK_EQUAL(rs, true);
  BOOST_CHECK_EQUAL(tested.count(), 0);
}

/**
 * @test Verify that the buy side of array_based_order_book works as
 * expected.
 * Works adding and removing prices at the limit of top level range
 * to verify move form and to the bottom level work as expected on those
 * border cases.
 */
BOOST_AUTO_TEST_CASE(array_based_order_book_sell_range) {
  using namespace jb::itch5;

  const std::size_t ticks = 5000;
  array_based_order_book::sells_t tested(
      array_based_order_book::config().max_size(2 * ticks));

  BOOST_CHECK_EQUAL(tested.count(), 0);
  // build a book around 1000*ticks
  // top levels range is 900*ticks by 1100*ticks
  auto rs = tested.add_order(price4_t(1000 * ticks), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // new inside, same range...
  rs = tested.add_order(price4_t(1000 * ticks - 100), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // a 3 prices(*) to test the move_to_bottom trigger...
  rs = tested.add_order(price4_t(1000 * ticks + 100), 100);
  BOOST_CHECK_EQUAL(rs, false);
  rs = tested.add_order(price4_t(1000 * ticks + 200), 100);
  BOOST_CHECK_EQUAL(rs, false);
  rs = tested.add_order(price4_t(1000 * ticks + 300), 100);
  BOOST_CHECK_EQUAL(rs, false);
  BOOST_CHECK_EQUAL(tested.count(), 5);

  // change the inside right below the limit
  rs = tested.add_order(price4_t(900 * ticks + 100), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // change the inside right at the limit
  // ... limit is excluded so it triggers the move to bottom
  rs = tested.add_order(price4_t(900 * ticks), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // change the inside
  rs = tested.add_order(price4_t(900 * ticks - 100), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // change the inside far above the limit
  // all prices move to bottom
  rs = tested.add_order(price4_t(100 * ticks), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // add a new price far below
  // in preparation to test move from bottom
  rs = tested.add_order(price4_t(1000 * ticks - 300), 100);
  BOOST_CHECK_EQUAL(rs, false);

  // add a new price far below
  rs = tested.add_order(price4_t(900 * ticks - 200), 100);
  BOOST_CHECK_EQUAL(rs, false);

  // remove the far above inside
  // all prices (but 3 prices (*)) are move from bottom back to top levels...
  rs = tested.reduce_order(price4_t(100 * ticks), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // now remove prices around 900*ticks to move the rest of the prices back...
  // removed the inside
  rs = tested.reduce_order(price4_t(900 * ticks - 200), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // removed the inside
  rs = tested.reduce_order(price4_t(900 * ticks - 100), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // removed the inside
  rs = tested.reduce_order(price4_t(900 * ticks), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // removed the inside, new inside right at the bottom of the range...
  // the rest of prices are moved from the bottom
  rs = tested.reduce_order(price4_t(900 * ticks + 100), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // removed the inside, new range
  rs = tested.reduce_order(price4_t(1000 * ticks - 300), 100);
  BOOST_CHECK_EQUAL(rs, true);

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
  BOOST_CHECK_EQUAL(tested.count(), 0);
}

/**
 * @test Verify that the buy side of array_based_order_book
 * works as expected.
 * Test suite for prices below $1.00. A smaller tick offset is used to
 * facilitate the tests
 * Works adding and removing prices at the limit of top level range
 * to verify move form and to the bottom level work as expected on those
 * border cases.
 */
BOOST_AUTO_TEST_CASE(array_based_order_book_buy_small_tick) {
  using namespace jb::itch5;

  array_based_order_book::buys_t tested(
      array_based_order_book::config().max_size(3000));

  // build a book around 15 cents
  // top level range is 0c (included) by 30c (excluded)
  auto rs = tested.add_order(price4_t(1500), 100);
  BOOST_CHECK_EQUAL(rs, true);

  rs = tested.add_order(price4_t(1501), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.add_order(price4_t(1502), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.add_order(price4_t(1499), 100);
  BOOST_CHECK_EQUAL(rs, false);
  rs = tested.add_order(price4_t(1498), 100);
  BOOST_CHECK_EQUAL(rs, false);

  // change the inside right below the limit
  // no prices moved...
  rs = tested.add_order(price4_t(2998), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // change the inside right at the limit
  // mo prices moved...
  rs = tested.add_order(price4_t(2999), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // change the inside right above the limit
  // new  range is 15c by 45c
  // .. prices below 15c are moved to the bottom...
  rs = tested.add_order(price4_t(3000), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // change the inside far above the limit
  // all prices, but the new inside, are moved to the bottom
  rs = tested.add_order(price4_t(9999), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // add order far below
  rs = tested.add_order(price4_t(3001), 100);
  BOOST_CHECK_EQUAL(rs, false);

  // remove the far above inside
  // new top levels range is 15.01c by 45.01c
  // so some prices are moved back from bottom levels...
  rs = tested.reduce_order(price4_t(9999), 100);
  BOOST_CHECK_EQUAL(rs, true);

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

  // remove inside at the bottom
  rs = tested.reduce_order(price4_t(1501), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // remove last 3 prices
  rs = tested.reduce_order(price4_t(1500), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.reduce_order(price4_t(1499), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.reduce_order(price4_t(1498), 100);
  BOOST_CHECK_EQUAL(rs, true);
  BOOST_CHECK_EQUAL(tested.count(), 0);
}

/**
 * @test Verify that the buy side of array_based_order_book
 * works as expected.
 * Test suite for prices below $1.00. A smaller tick offset is used to
 * facilitate the tests
 * Works adding and removing prices at the limit of top level range
 * to verify move form and to the bottom level work as expected on those
 * border cases.
 */
BOOST_AUTO_TEST_CASE(array_based_order_book_sell_small_tick) {
  using namespace jb::itch5;

  array_based_order_book::sells_t tested(
      array_based_order_book::config().max_size(3000));

  // build a book around 75 cents
  // top levels range is 60c by 90c
  auto rs = tested.add_order(price4_t(7500), 100);
  BOOST_CHECK_EQUAL(rs, true);

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

  // change the inside far above the limit
  // move all prices, but the new inside, to the bottom levels...
  rs = tested.add_order(price4_t(989), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // add order far below
  rs = tested.add_order(price4_t(5998), 100);
  BOOST_CHECK_EQUAL(rs, false);

  // remove the far above inside
  // prices are move back from the bottom...
  rs = tested.reduce_order(price4_t(989), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // remove price to test
  rs = tested.reduce_order(price4_t(5999), 100);
  BOOST_CHECK_EQUAL(rs, false);
  rs = tested.reduce_order(price4_t(6000), 100);
  BOOST_CHECK_EQUAL(rs, false);

  // remove the inside
  // the rest of the prices is moved back to the top levels
  rs = tested.reduce_order(price4_t(5998), 100);
  BOOST_CHECK_EQUAL(rs, true);

  rs = tested.reduce_order(price4_t(7498), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // remove last 4 prices
  rs = tested.reduce_order(price4_t(7499), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.reduce_order(price4_t(7500), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.reduce_order(price4_t(7501), 100);
  BOOST_CHECK_EQUAL(rs, true);
  rs = tested.reduce_order(price4_t(7502), 100);
  BOOST_CHECK_EQUAL(rs, true);
  BOOST_CHECK_EQUAL(tested.count(), 0);
}

/**
 * @test itch5arrayinside exception generated. Adding test case to fix the
 * problem.
 */
BOOST_AUTO_TEST_CASE(array_based_order_book_sell_small_tick_bug01) {
  using namespace jb::itch5;

  const std::size_t ticks = 5000;
  array_based_order_book::sells_t tested(
      array_based_order_book::config().max_size(2 * ticks));

  // add shares 100 @199999.9900
  auto rs = tested.add_order(price4_t(1999999900), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // add 100  @0.5850
  rs = tested.add_order(price4_t(5850), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // add +100 (200 now)  @0.5850
  rs = tested.add_order(price4_t(5850), 100);
  BOOST_CHECK_EQUAL(rs, true);
  // remove shares 100 @199999.9900
  rs = tested.reduce_order(price4_t(1999999900), 100);
  BOOST_CHECK_EQUAL(rs, false);
  // add shares 100 @199999.9900
  rs = tested.add_order(price4_t(1999999900), 100);
  BOOST_CHECK_EQUAL(rs, false);

  // add 100 @0.0130 (px_inside_ now)
  rs = tested.add_order(price4_t(130), 100);
  BOOST_CHECK_EQUAL(rs, true);

  // add 500 @0.0150
  rs = tested.add_order(price4_t(150), 100);
  BOOST_CHECK_EQUAL(rs, false);
  // remove shares 100 @0.5850
  rs = tested.reduce_order(price4_t(5850), 100);
  BOOST_CHECK_EQUAL(rs, false);
}

/**
 * @test Verify that array_based_order_book::config works as expected.
 */
BOOST_AUTO_TEST_CASE(array_based_order_book_config_simple) {
  using namespace jb::itch5;
  BOOST_CHECK_NO_THROW(array_based_order_book::config().validate());
  BOOST_CHECK_THROW(
      array_based_order_book::config().max_size(-7).validate(), jb::usage);
  BOOST_CHECK_NO_THROW(
      array_based_order_book::config().max_size(3000).validate());
}
