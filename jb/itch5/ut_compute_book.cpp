#include <jb/itch5/compute_book.hpp>
#include <jb/itch5/testing/messages.hpp>
#include <jb/as_hhmmss.hpp>

#include <skye/mock_function.hpp>
#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <thread>

using namespace jb::itch5;

/**
 * Helper functions and constants to test jb::itch5::compute_book.
 */
namespace {
buy_sell_indicator_t const BUY(u'B');
buy_sell_indicator_t const SELL(u'S');
} // anonymous namespace

/**
 * @test Verify that jb::itch5::compute_book::handle_message works as
 * expected for add_order_message BUY.
 */
BOOST_AUTO_TEST_CASE(compute_book_add_order_message_buy) {
  skye::mock_function<void(
      compute_book::book_update update, half_quote best_bid,
      half_quote best_offer, int buy_count, int offer_count)>
      callback;
  auto cb = [&callback](
      message_header const&, order_book const& b,
      compute_book::book_update const& update) {
    callback(
        update, b.best_bid(), b.best_offer(), b.buy_count(), b.sell_count());
  };

  compute_book tested(cb);

  using namespace jb::itch5::testing;

  stock_t const stock("HSART");
  price4_t const p10(100000);
  price4_t const p11(110000);
  price4_t const p09(90000);
  long msgcnt = 0;
  std::uint64_t id = 2;
  compute_book::time_point now = tested.now();
  // ... add an initial order to the book ...
  tested.handle_message(
      now, ++msgcnt, 0,
      add_order_message{{add_order_message::message_type, 0, 0,
                         timestamp{std::chrono::nanoseconds(0)}},
                        ++id,
                        BUY,
                        100,
                        stock,
                        p10});
  // ... we expect the new order to implicitly add the symbol ...
  auto symbols = tested.symbols();
  BOOST_REQUIRE_EQUAL(symbols.size(), std::size_t(1));
  BOOST_CHECK_EQUAL(symbols[0], stock);

  // ... we also expect a single update, and the order should be in
  // the book when the update is called ...
  callback.check_called().once().with(
      compute_book::book_update{now, stock, BUY, p10, 100},
      half_quote{p10, 100}, order_book::empty_offer(), 1, 0);

  // ... add an order at a better price ...
  now = tested.now();
  tested.handle_message(
      now, ++msgcnt, 0,
      add_order_message{{add_order_message::message_type, 0, 0,
                         timestamp{std::chrono::nanoseconds(0)}},
                        ++id,
                        BUY,
                        200,
                        stock,
                        p11});

  // ... we also expect a single update, and the order should be in
  // the book when the update is called ...
  callback.check_called().once().with(
      compute_book::book_update{now, stock, BUY, p11, 200},
      half_quote{p11, 200}, order_book::empty_offer(), 2, 0);

  // ... add an order at a worse price ...
  now = tested.now();
  tested.handle_message(
      now, ++msgcnt, 0,
      add_order_message{{add_order_message::message_type, 0, 0,
                         timestamp{std::chrono::nanoseconds(0)}},
                        ++id,
                        BUY,
                        300,
                        stock,
                        p09});

  // ... we also expect a single update, and the order should be in
  // the book when the update is called ...
  callback.check_called().once().with(
      compute_book::book_update{now, stock, BUY, p09, 300},
      half_quote{p11, 200}, order_book::empty_offer(), 3, 0);

  // ... those should be all the updates, regardless of their contents
  // ...
  callback.check_called().exactly(3);
}

/**
 * @test Verify that jb::itch5::compute_book::handle_message works as
 * expected for add_order_message SELL.
 */
BOOST_AUTO_TEST_CASE(compute_book_add_order_message_sell) {
  skye::mock_function<void(
      compute_book::book_update update, half_quote best_bid,
      half_quote best_offer, int buy_count, int offer_count)>
      callback;
  auto cb = [&callback](
      message_header const&, order_book const& b,
      compute_book::book_update const& update) {
    callback(
        update, b.best_bid(), b.best_offer(), b.buy_count(), b.sell_count());
  };

  compute_book tested(cb);

  using namespace jb::itch5::testing;

  stock_t const stock("HSART");
  price4_t const p10(100000);
  price4_t const p11(110000);
  price4_t const p12(120000);
  long msgcnt = 0;
  std::uint64_t id = 2;
  compute_book::time_point now = tested.now();
  // ... add an initial order to the book ...
  tested.handle_message(
      now, ++msgcnt, 0,
      add_order_message{{add_order_message::message_type, 0, 0,
                         timestamp{std::chrono::nanoseconds(0)}},
                        ++id,
                        SELL,
                        100,
                        stock,
                        p11});
  // ... we expect the new order to implicitly add the symbol ...
  auto symbols = tested.symbols();
  BOOST_REQUIRE_EQUAL(symbols.size(), std::size_t(1));
  BOOST_CHECK_EQUAL(symbols[0], stock);

  // ... we also expect a single update, and the order should be in
  // the book when the update is called ...
  callback.check_called().once().with(
      compute_book::book_update{now, stock, SELL, p11, 100},
      order_book::empty_bid(), half_quote{p11, 100}, 0, 1);

  // ... add an order at a better price ...
  now = tested.now();
  tested.handle_message(
      now, ++msgcnt, 0,
      add_order_message{{add_order_message::message_type, 0, 0,
                         timestamp{std::chrono::nanoseconds(0)}},
                        ++id,
                        SELL,
                        200,
                        stock,
                        p10});

  // ... we also expect a single update, and the order should be in
  // the book when the update is called ...
  callback.check_called().once().with(
      compute_book::book_update{now, stock, SELL, p10, 200},
      order_book::empty_bid(), half_quote{p10, 200}, 0, 2);

  // ... add an order at a worse price ...
  now = tested.now();
  tested.handle_message(
      now, ++msgcnt, 0,
      add_order_message{{add_order_message::message_type, 0, 0,
                         timestamp{std::chrono::nanoseconds(0)}},
                        ++id,
                        SELL,
                        300,
                        stock,
                        p12});

  // ... we also expect a single update, and the order should be in
  // the book when the update is called ...
  callback.check_called().once().with(
      compute_book::book_update{now, stock, SELL, p12, 300},
      order_book::empty_bid(), half_quote{p10, 200}, 0, 3);

  // ... those should be all the updates, regardless of their contents
  // ...
  callback.check_called().exactly(3);
}

/**
 * @test Increase code coverage in
 * jb::itch5::compute_book::handle_message for add_order_message.
 */
BOOST_AUTO_TEST_CASE(compute_book_add_order_message_edge_cases) {
  skye::mock_function<void()> callback;
  auto cb = [&callback](
      message_header const&, order_book const& b,
      compute_book::book_update const& update) { callback(); };

  compute_book tested(cb);

  using namespace jb::itch5::testing;

  stock_t const stock("HSART");
  price4_t const p10(100000);
  long msgcnt = 0;
  std::uint64_t id = 2;
  compute_book::time_point now = tested.now();
  // ... add an initial order to the book ...
  tested.handle_message(
      now, ++msgcnt, 0,
      add_order_message{{add_order_message::message_type, 0, 0,
                         timestamp{std::chrono::nanoseconds(0)}},
                        ++id,
                        BUY,
                        100,
                        stock,
                        p10});
  // ... we expect a callback from that event ...
  callback.check_called().once();

  // ... try to send the same order ...
  tested.handle_message(
      now, ++msgcnt, 0,
      add_order_message{{add_order_message::message_type, 0, 0,
                         timestamp{std::chrono::nanoseconds(0)}},
                        id,
                        BUY,
                        100,
                        stock,
                        p10});
  // ... we expect no additional callbacks in this case ...
  callback.check_called().once();
}

/**
*@test Verify that jb::itch5::compute_book::handle_message works as
*expected for stock_directory_message.
*/
BOOST_AUTO_TEST_CASE(compute_book_stock_directory_message) {
  skye::mock_function<void()> callback;
  auto cb = [&callback](
      message_header const&, order_book const&,
      compute_book::book_update const& update) { callback(); };

  compute_book tested(cb);

  using namespace jb::itch5::testing;

  compute_book::time_point now = tested.now();
  long msgcnt = 0;
  tested.handle_message(now, ++msgcnt, 0, create_stock_directory("HSART"));
  tested.handle_message(now, ++msgcnt, 0, create_stock_directory("FOO"));
  tested.handle_message(now, ++msgcnt, 0, create_stock_directory("B"));

  std::vector<stock_t> expected{stock_t("HSART"), stock_t("FOO"), stock_t("B")};
  auto actual = tested.symbols();
  // ... the symbols are not guaranteed to return in any particular
  // order, so sort them to make testing easier ...
  std::sort(expected.begin(), expected.end());
  std::sort(actual.begin(), actual.end());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      expected.begin(), expected.end(), actual.begin(), actual.end());

  // ... a repeated symbol should have no effect ...
  tested.handle_message(now, ++msgcnt, 0, create_stock_directory("B"));
  actual = tested.symbols();
  std::sort(expected.begin(), expected.end());
  std::sort(actual.begin(), actual.end());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      expected.begin(), expected.end(), actual.begin(), actual.end());
}

/**
 * @test Verify that jb::itch5::compute_book::book_update operators
 * work as expected.
 */
BOOST_AUTO_TEST_CASE(compute_book_book_update_operators) {
  auto const ts0 = compute_book::clock_type::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  auto const ts1 = compute_book::clock_type::now();
  BOOST_CHECK_EQUAL(
      compute_book::book_update({ts0, stock_t("A"), BUY, price4_t(1000), 100}),
      compute_book::book_update({ts0, stock_t("A"), BUY, price4_t(1000), 100}));

  BOOST_CHECK_NE(
      compute_book::book_update({ts0, stock_t("A"), BUY, price4_t(1000), 100}),
      compute_book::book_update({ts1, stock_t("A"), BUY, price4_t(1000), 100}));
  BOOST_CHECK_NE(
      compute_book::book_update({ts0, stock_t("A"), BUY, price4_t(1000), 100}),
      compute_book::book_update({ts0, stock_t("B"), BUY, price4_t(1000), 100}));
  BOOST_CHECK_NE(
      compute_book::book_update({ts0, stock_t("A"), BUY, price4_t(1000), 10}),
      compute_book::book_update({ts0, stock_t("A"), SELL, price4_t(1000), 10}));
  BOOST_CHECK_NE(
      compute_book::book_update({ts0, stock_t("A"), BUY, price4_t(1000), 100}),
      compute_book::book_update({ts0, stock_t("A"), BUY, price4_t(1001), 100}));
  BOOST_CHECK_NE(
      compute_book::book_update({ts0, stock_t("A"), BUY, price4_t(1000), 100}),
      compute_book::book_update({ts0, stock_t("B"), BUY, price4_t(1000), 200}));
}
