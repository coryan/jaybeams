#include <jb/itch5/order_book_depth.hpp>

#include <boost/test/unit_test.hpp>

  /* Ticket https://github.com/coryan/jaybeams/issues/20
   *
   * Remove old test (duplicated from ut_order_book.cpp
   * Add tests to verify class handles Book Depth correctly
   *
   */

BOOST_AUTO_TEST_CASE(order_book_trivial) {
  jb::itch5::order_book_depth tested;
  //  book_depth should be 0
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 0);  
}

BOOST_AUTO_TEST_CASE(order_book_buy) {
  using jb::itch5::price4_t;
  jb::itch5::order_book_depth tested;

  jb::itch5::buy_sell_indicator_t const BUY(u'B');
  jb::itch5::buy_sell_indicator_t const SELL(u'S');

  // Add a new order ...
  bool r = tested.handle_add_order(BUY, price4_t(100000), 100);
  // handler should return true... it is an event
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 1);  

  // add a new buy order, new price...
  r = tested.handle_add_order(BUY, price4_t(99900), 300);
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 2);  
  
  // ... update at the bid increases the qty... is an event
  r = tested.handle_add_order(BUY, price4_t(100000), 400);
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. but the book_depth should not be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 2);  
  
  // ... a better price changes both price and qty ...
  r = tested.handle_add_order(BUY, price4_t(100100), 200);
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 3);  

  // ... decrease below the bid has no effect ...
  r = tested.handle_order_reduced(BUY, price4_t(100000), 400);
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should not be decremented since (100 remaining) 
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 3);
  
  // ... even when it is over the existing quantity ...
  r = tested.handle_order_reduced(BUY, price4_t(100000), 200);
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should be decremented since (<=0 remaining) 
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 2);
  
  // ... deleting the best bid uncovers the best price ...
  r = tested.handle_order_reduced(BUY, price4_t(100100), 200);
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should be decremented since (=0 remaining) 
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 1);

  // ... trying to delete previous bid again ...
  r = tested.handle_order_reduced(BUY, price4_t(100100), 200);
  // handler should return false, since is not a new event (no changes to the book)
  BOOST_CHECK_EQUAL(r, false);  
  // .. and the book_depth should not be decremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 1);
}

BOOST_AUTO_TEST_CASE(order_book_sell) {
  using jb::itch5::price4_t;
  jb::itch5::order_book_depth tested;

  jb::itch5::buy_sell_indicator_t const BUY(u'B');
  jb::itch5::buy_sell_indicator_t const SELL(u'S');

  // Add a new order ...
  bool r = tested.handle_add_order(SELL, price4_t(100000), 100);
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 1);

  // ... adding above the best offer has no effect ...
  r = tested.handle_add_order(SELL, price4_t(100100), 300);
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 2);

  // ... update at the offer increases the qty ...
  r = tested.handle_add_order(SELL, price4_t(100000), 400);
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should not be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 2);

  // ... a better price changes both price and qty ...
  r = tested.handle_add_order(SELL, price4_t(99900), 200);
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 3);

  // ... decrease above the offer has no effect ...
  r = tested.handle_order_reduced(SELL, price4_t(100000), 400);
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should not be incremented (100 remaining)
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 3);

  // ... even when it is over the existing quantity ...
  r = tested.handle_order_reduced(SELL, price4_t(100000), 200);
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should be decremented (none remaining)
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 2);

  // ... deleting the best offer uncovers the best price ...
  r = tested.handle_order_reduced(SELL, price4_t(99900), 200);
  // handler should return true...
  BOOST_CHECK_EQUAL(r, true);  
  // .. and the book_depth should be decremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 1);

  // ... trying to delete the same bid again should not affect
  r = tested.handle_order_reduced(SELL, price4_t(99900), 200);
  // handler should return false, since it is not an event...
  BOOST_CHECK_EQUAL(r, false);  
  // .. and the book_depth should not be decremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 1);
}
