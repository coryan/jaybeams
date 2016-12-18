#include <jb/itch5/order_book.hpp>

#include <boost/test/unit_test.hpp>

namespace jb {
namespace itch5 {
namespace testing {

/**
 * Test order book trivial members.
 */
template <typename book_type>
void test_order_book_trivial(book_type& tested) {
  using jb::itch5::price4_t;

  auto actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(0));
  BOOST_CHECK_EQUAL(actual.second, 0);
  actual = tested.worst_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(0));
  BOOST_CHECK_EQUAL(actual.second, 0);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(200000UL * 10000));
  BOOST_CHECK_EQUAL(actual.second, 0);
  actual = tested.worst_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(200000UL * 10000));
  BOOST_CHECK_EQUAL(actual.second, 0);
  //  book_depth should be 0
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 0);
}

/**
 * Test order book buy side order handling.
 */
template <typename book_type>
void test_order_book_buy_order_handling(book_type& tested) {
  using jb::itch5::price4_t;

  jb::itch5::buy_sell_indicator_t const BUY(u'B');

  // Add a new order ...
  auto r = tested.handle_add_order(BUY, price4_t(100000), 100);
  // ... the offer should not change ...
  auto actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(200000UL * 10000));
  BOOST_CHECK_EQUAL(actual.second, 0);
  actual = tested.worst_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(200000UL * 10000));
  BOOST_CHECK_EQUAL(actual.second, 0);
  // .. but the bid should ...
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);
  actual = tested.worst_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);
  // handler should return true... it is an inside change
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 1);

  // ... adding below the best bid has no effect ...
  r = tested.handle_add_order(BUY, price4_t(99900), 300);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);
  actual = tested.worst_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 300);
  // handler should return false
  BOOST_CHECK_EQUAL(r, false);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 2);

  // ... update at the bid increases the qty ...
  r = tested.handle_add_order(BUY, price4_t(100000), 400);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 500);
  // handler should return true... it is an inside change
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should not be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 2);

  // ... a better price changes both price and qty ...
  r = tested.handle_add_order(BUY, price4_t(100100), 200);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 200);
  // handler should return true... it is an inside change
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 3);

  // ... decrease below the bid has no effect ...
  r = tested.handle_order_reduced(BUY, price4_t(100000), 400);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 200);
  // handler should return false
  BOOST_CHECK_EQUAL(r, false);
  // .. and the book_depth should not be decremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 3);

  // ... even when it is over the existing quantity ...
  r = tested.handle_order_reduced(BUY, price4_t(100000), 200);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 200);
  // handler should return false
  BOOST_CHECK_EQUAL(r, false);
  // .. and the book_depth should be decremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 2);

  // ... deleting the best bid uncovers the best price ...
  r = tested.handle_order_reduced(BUY, price4_t(100100), 200);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 300);
  // handler should return true... it is an inside change
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should be decremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 1);

  // ... deleting the remaining price takes the book depth to 0
  r = tested.handle_order_reduced(BUY, price4_t(99900), 300);
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(0));
  BOOST_CHECK_EQUAL(actual.second, 0);
  // handler should return true... it is an inside change
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should be decremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 0);
}

/**
 * Test order book sell side order handling.
 */
template <typename book_type>
void test_order_book_sell_order_handling(book_type& tested) {
  using jb::itch5::price4_t;

  jb::itch5::buy_sell_indicator_t const SELL(u'S');

  // Add a new order ...
  auto r = tested.handle_add_order(SELL, price4_t(100000), 100);
  // ... the offer should not change ...
  auto actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(0));
  BOOST_CHECK_EQUAL(actual.second, 0);
  actual = tested.worst_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(0));
  BOOST_CHECK_EQUAL(actual.second, 0);
  // .. but the offer should ...
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);
  actual = tested.worst_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);
  // handler should return true... it is an inside change
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 1);

  // ... adding above the best offer has no effect ...
  r = tested.handle_add_order(SELL, price4_t(100100), 300);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 100);
  // ... the worst offer should change though ...
  actual = tested.worst_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 300);
  // handler should return false
  BOOST_CHECK_EQUAL(r, false);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 2);

  // ... update at the offer increases the qty ...
  r = tested.handle_add_order(SELL, price4_t(100000), 400);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100000));
  BOOST_CHECK_EQUAL(actual.second, 500);
  // handler should return true... it is an inside change
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should not change
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 2);

  // ... a better price changes both price and qty ...
  r = tested.handle_add_order(SELL, price4_t(99900), 200);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 200);
  // handler should return true... it is an inside change
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 3);

  // ... decrease above the offer has no effect ...
  r = tested.handle_order_reduced(SELL, price4_t(100000), 400);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 200);
  // handler should return false
  BOOST_CHECK_EQUAL(r, false);
  // .. and the book_depth should not change
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 3);

  // ... even when it is over the existing quantity ...
  r = tested.handle_order_reduced(SELL, price4_t(100000), 200);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(99900));
  BOOST_CHECK_EQUAL(actual.second, 200);
  // handler should return false
  BOOST_CHECK_EQUAL(r, false);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 2);

  // ... deleting the best offer uncovers the best price ...
  r = tested.handle_order_reduced(SELL, price4_t(99900), 200);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(100100));
  BOOST_CHECK_EQUAL(actual.second, 300);
  // handler should return true... it is an inside change
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should be incremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 1);

  // ... deleting the remaining price takes the book depth to 0
  r = tested.handle_order_reduced(SELL, price4_t(100100), 300);
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(200000UL * 10000));
  BOOST_CHECK_EQUAL(actual.second, 0);
  // handler should return true
  BOOST_CHECK_EQUAL(r, true);
  // .. and the book_depth should be decremented
  BOOST_CHECK_EQUAL(tested.get_book_depth(), 0);
}

/**
 * Test order book error conditions.
 */
template <typename book_type>
void test_order_book_errors(book_type& tested) {
  using jb::itch5::price4_t;

  jb::itch5::buy_sell_indicator_t const BUY(u'B');
  jb::itch5::buy_sell_indicator_t const SELL(u'S');

  // Add two orders to the book ...
  (void)tested.handle_add_order(BUY, price4_t(100000), 100);
  (void)tested.handle_add_order(BUY, price4_t(110000), 200);

  // ... check the best bid ...
  auto actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(110000));
  BOOST_CHECK_EQUAL(actual.second, 200);

  // ... remove the first order, once should work, the second time
  // should fail ...
  tested.handle_order_reduced(BUY, price4_t(100000), 100);
  BOOST_CHECK_THROW(
      tested.handle_order_reduced(BUY, price4_t(100000), 100), std::exception);

  // ... check the best bid ...
  actual = tested.best_bid();
  BOOST_CHECK_EQUAL(actual.first, price4_t(110000));
  BOOST_CHECK_EQUAL(actual.second, 200);

  // Add two orders to the book ...
  (void)tested.handle_add_order(SELL, price4_t(120000), 100);
  (void)tested.handle_add_order(SELL, price4_t(110000), 200);

  // ... check the best offer ...
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(110000));
  BOOST_CHECK_EQUAL(actual.second, 200);

  // ... remove the first order, once should work, the second time
  // should fail ...
  tested.handle_order_reduced(SELL, price4_t(120000), 100);
  BOOST_CHECK_THROW(
      tested.handle_order_reduced(SELL, price4_t(120000), 100), std::exception);

  // ... check the best offer ...
  actual = tested.best_offer();
  BOOST_CHECK_EQUAL(actual.first, price4_t(110000));
  BOOST_CHECK_EQUAL(actual.second, 200);
}

} // namespace testing
} // namespace itch5
} // namespace testing

/**
 * @test Verify that order_book::config works as expected.
 */
BOOST_AUTO_TEST_CASE(order_book_config_simple) {
  using config =
      jb::itch5::order_book<jb::itch5::array_based_order_book>::config;

  BOOST_CHECK_NO_THROW(config().validate());
  BOOST_CHECK_THROW(config().max_size(-7).validate(), jb::usage);
  BOOST_CHECK_NO_THROW(config().max_size(3000).validate());
  BOOST_CHECK_THROW(config().max_size(20000).validate(), jb::usage);
}

/**
 * @test Verify that jb::itch5::order_book<jb::itch5::map_price> works as
 * expected.
 */
BOOST_AUTO_TEST_CASE(order_book_trivial) {
  using namespace jb::itch5;
  using map_book_type = order_book<map_based_order_book>;

  map_book_type::config map_cfg;
  map_book_type map_tested(map_cfg);
  testing::test_order_book_trivial(map_tested);

  using array_book_type = order_book<array_based_order_book>;
  array_book_type::config array_cfg;
  array_book_type array_tested(array_cfg);
  testing::test_order_book_trivial(array_tested);
}

/**
 * @test Verify that buy side of order_book<book_type>
 * works as expected.
 */
BOOST_AUTO_TEST_CASE(order_book_buy) {
  using namespace jb::itch5;
  using map_book_type = order_book<map_based_order_book>;

  map_book_type::config map_cfg;
  map_book_type map_tested(map_cfg);
  testing::test_order_book_buy_order_handling(map_tested);

  using array_book_type = order_book<array_based_order_book>;
  // uses default max_size
  array_book_type::config array_cfg;
  array_book_type array_tested(array_cfg);
  testing::test_order_book_buy_order_handling(array_tested);

  // defines max_size to 3000
  array_book_type sh_array_tested(array_book_type::config().max_size(3000));
  testing::test_order_book_buy_order_handling(sh_array_tested);
}

/**
 * @test Verity that the sell side of
 * jb::itch5::order_book<jb::itch5::map_price> works as expected.
 */
BOOST_AUTO_TEST_CASE(order_book_sell) {
  using namespace jb::itch5;
  using map_book_type = order_book<map_based_order_book>;

  map_book_type::config map_cfg;
  map_book_type map_tested(map_cfg);
  testing::test_order_book_sell_order_handling(map_tested);

  using array_book_type = order_book<array_based_order_book>;
  // uses default max_size
  array_book_type::config array_cfg;
  array_book_type array_tested(array_cfg);
  testing::test_order_book_sell_order_handling(array_tested);

  // defines max_size to 3000
  array_book_type sh_array_tested(array_book_type::config().max_size(3000));
  jb::itch5::testing::test_order_book_sell_order_handling(sh_array_tested);
}

/**
 * @test Verify that order_book handles errors as expected.
 */
BOOST_AUTO_TEST_CASE(order_book_errors) {
  using namespace jb::itch5;
  using map_book_type = order_book<map_based_order_book>;

  map_book_type::config map_cfg;
  map_book_type map_tested(map_cfg);
  testing::test_order_book_errors(map_tested);

  using array_book_type = order_book<array_based_order_book>;
  array_book_type::config array_cfg;
  array_book_type array_tested(array_cfg);
  testing::test_order_book_errors(array_tested);
}
