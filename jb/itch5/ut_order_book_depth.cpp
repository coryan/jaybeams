#include <jb/itch5/order_book_depth.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::itch5::order_book_depth works as expected.
 */
BOOST_AUTO_TEST_CASE(order_book_trivial) {
  using jb::itch5::price4_t;
  jb::itch5::order_book_depth tested;

  auto actual = tested.best_bid();
  // actual should be empty_bid()
  BOOST_CHECK_EQUAL(actual.first, price4_t(0));    
  BOOST_CHECK_EQUAL(actual.second, 0);
  actual = tested.best_offer();
  // actual should be empty_offer()
  BOOST_CHECK_EQUAL(actual.first, price4_t(200000UL * 10000));
  BOOST_CHECK_EQUAL(actual.second, 0);
  // Ticket #?001 : implements order_book_depth
  // book_depth should be 0
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 0);  
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
  bool r = tested.handle_add_order(BUY, price4_t(100000), 100);
  // ... the offer should not change ...
  auto actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(200000UL * 10000));
  BOOST_CHECK_EQUAL(actual.second, 0);

  // .. but the bid should ...
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);

  // Ticket #?001 : implements order_book_depth
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 1);  
  
  // ... adding below the best bid has no effect
  // on the best bid...
  r = tested.handle_add_order(BUY, price4_t(99900), 300);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);
  // Ticket #?001 : implements order_book_depth
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 2);  
  
  // ... update at the bid increases the qty ...
  r = tested.handle_add_order(BUY, price4_t(100000), 400);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 500);
  // Ticket #?001 : implements order_book_depth
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should not be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 2);  
  
  // ... a better price changes both price and qty ...
  r = tested.handle_add_order(BUY, price4_t(100100), 200);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 200);
  // Ticket #?001 : implements order_book_depth
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 3);  

  // ... decrease below the bid has no effect ...
  r = tested.handle_order_reduced(BUY, price4_t(100000), 400);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 200);
  // Ticket #?001 : implements order_book_depth
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should not be decremented since (100 remaining) 
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 3);
  
  // ... even when it is over the existing quantity ...
  r = tested.handle_order_reduced(BUY, price4_t(100000), 200);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 200);
  // Ticket #?001 : implements order_book_depth
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should be decremented since (<=0 remaining) 
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 2);
  
  // ... deleting the best bid uncovers the best price ...
  r = tested.handle_order_reduced(BUY, price4_t(100100), 200);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 300);
  // Ticket #?001 : implements order_book_depth
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should be decremented since (=0 remaining) 
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 1);

  // Ticket #?001 : implements order_book_depth
  // ... trying to delete previous bid again ...
  r = tested.handle_order_reduced(BUY, price4_t(100100), 200);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 300);
  // handler should return false, since is not a new event (no changes to the book)
  BOOST_CHECK_EQUAL(r, false);  
  // .. and the book_depth should not be decremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 1);
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
  bool r = tested.handle_add_order(SELL, price4_t(100000), 100);
  // ... the offer should not change ...
  auto actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(0));
  BOOST_CHECK_EQUAL(actual.second, 0);
  // .. but the offer should ...
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);
  // Ticket #?001 : implements order_book_depth
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 1);

  // ... adding above the best offer has no effect ...
  r = tested.handle_add_order(SELL, price4_t(100100), 300);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);
  // Ticket #?001 : implements order_book_depth
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 2);

  // ... update at the offer increases the qty ...
  r = tested.handle_add_order(SELL, price4_t(100000), 400);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 500);
  // Ticket #?001 : implements order_book_depth
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should not be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 2);

  // ... a better price changes both price and qty ...
  r = tested.handle_add_order(SELL, price4_t(99900), 200);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 200);
  // Ticket #?001 : implements order_book_depth
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 3);

  // ... decrease above the offer has no effect ...
  r = tested.handle_order_reduced(SELL, price4_t(100000), 400);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 200);
  // Ticket #?001 : implements order_book_depth
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should not be incremented (100 remaining)
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 3);

  // ... even when it is over the existing quantity ...
  r = tested.handle_order_reduced(SELL, price4_t(100000), 200);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 200);
  // Ticket #?001 : implements order_book_depth
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should be decremented (none remaining)
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 2);

  // ... deleting the best offer uncovers the best price ...
  r = tested.handle_order_reduced(SELL, price4_t(99900), 200);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 300);
  // Ticket #?001 : implements order_book_depth
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should be decremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 1);

  // ... trying to delete the same bid again should not affect
  r = tested.handle_order_reduced(SELL, price4_t(99900), 200);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 300);
  // Ticket #?001 : implements order_book_depth
  // handler should return false, since it is not an event...
  BOOST_CHECK_EQUAL(r, false);  
  // .. and the book_depth should not be decremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 1);
}
