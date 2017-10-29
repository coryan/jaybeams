#include <jb/itch5/compute_book.hpp>
#include <jb/itch5/testing/data.hpp>
#include <jb/itch5/testing/messages.hpp>
#include <jb/itch5/trade_message.hpp>
#include <jb/as_hhmmss.hpp>

#include <jb/gmock/init.hpp>
#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <thread>

namespace jb {
namespace itch5 {
namespace testing {
buy_sell_indicator_t const BUY(u'B');
buy_sell_indicator_t const SELL(u'S');

struct mock_book_callback {
  MOCK_METHOD5(
      exec, void(
                book_update update, half_quote best_bid, half_quote best_offer,
                int buy_count, int offer_count));
};

/**
 * Test compute book based on book type.
 * @tparam based_order_book type used by compute_order and order_book
 */
template <typename based_order_book>
void test_compute_book_add_order_message_buy() {
  using book_type = order_book<based_order_book>;
  using compute_type = compute_book<based_order_book>;
  ::testing::StrictMock<mock_book_callback> callback;

  auto cb = [&callback](
      jb::itch5::message_header const&, book_type const& b,
      book_update const& update) {
    callback.exec(
        update, b.best_bid(), b.best_offer(), b.buy_count(), b.sell_count());
  };

  typename based_order_book::config cfg;
  compute_type tested(cb, cfg);

  stock_t const stock("HSART");
  price4_t const p10(100000);
  price4_t const p11(110000);
  price4_t const p09(90000);
  long msgcnt = 0;
  std::uint64_t id = 2;
  time_point now = tested.now();

  // ... add an initial order to the book, we expect a single update,
  // and the order should be in the book when the update is called ...
  EXPECT_CALL(
      callback, exec(
                    book_update{now, stock, BUY, p10, 100},
                    half_quote{p10, 100}, empty_offer(), 1, 0))
      .Times(1);
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

  // ... add an order at a better price, we also expect a single
  // update, and the order should be in the book when the update is
  // called ...
  now = tested.now();
  EXPECT_CALL(
      callback, exec(
                    book_update{now, stock, BUY, p11, 200},
                    half_quote{p11, 200}, empty_offer(), 2, 0))
      .Times(1);
  tested.handle_message(
      now, ++msgcnt, 0,
      add_order_message{{add_order_message::message_type, 0, 0,
                         timestamp{std::chrono::nanoseconds(0)}},
                        ++id,
                        BUY,
                        200,
                        stock,
                        p11});

  // ... add an order at a worse price, we also expect a single update, and the
  // order should be in the book when the update is called ...
  now = tested.now();
  EXPECT_CALL(
      callback, exec(
                    book_update{now, stock, BUY, p09, 300},
                    half_quote{p11, 200}, empty_offer(), 3, 0))
      .Times(1);
  tested.handle_message(
      now, ++msgcnt, 0,
      add_order_message{{add_order_message::message_type, 0, 0,
                         timestamp{std::chrono::nanoseconds(0)}},
                        ++id,
                        BUY,
                        300,
                        stock,
                        p09});
}

/**
 * Test compute book based on book type.
 * @tparam based_order_book type used by compute_book and order_book
 */
template <typename based_order_book>
void test_compute_book_add_order_message_sell() {
  using book_type = order_book<based_order_book>;
  using compute_type = compute_book<based_order_book>;

  ::testing::StrictMock<mock_book_callback> callback;
  auto cb = [&callback](
      jb::itch5::message_header const&, book_type const& b,
      book_update const& update) {
    callback.exec(
        update, b.best_bid(), b.best_offer(), b.buy_count(), b.sell_count());
  };

  typename based_order_book::config cfg;
  book_type book(cfg);
  compute_type tested(cb, cfg);

  stock_t const stock("HSART");
  price4_t const p10(100000);
  price4_t const p11(110000);
  price4_t const p12(120000);
  long msgcnt = 0;
  std::uint64_t id = 2;
  time_point now = tested.now();

  // ... add an initial order to the book, we expect a single update,
  // and the order should be in the book when the update is called ...
  EXPECT_CALL(
      callback, exec(
                    book_update{now, stock, SELL, p11, 100}, empty_bid(),
                    half_quote{p11, 100}, 0, 1))
      .Times(1);
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

  // ... add an order at a better price, we  expect a single update,
  // and the order should be in the book when the update is called ...
  now = tested.now();
  EXPECT_CALL(
      callback, exec(
                    book_update{now, stock, SELL, p10, 200}, empty_bid(),
                    half_quote{p10, 200}, 0, 2))
      .Times(1);
  tested.handle_message(
      now, ++msgcnt, 0,
      add_order_message{{add_order_message::message_type, 0, 0,
                         timestamp{std::chrono::nanoseconds(0)}},
                        ++id,
                        SELL,
                        200,
                        stock,
                        p10});

  // ... add an order at a worse price, we also expect a single
  // update, and the order should be in the book when the update is
  // called ...
  now = tested.now();
  EXPECT_CALL(
      callback, exec(
                    book_update{now, stock, SELL, p12, 300}, empty_bid(),
                    half_quote{p10, 200}, 0, 3))
      .Times(1);
  tested.handle_message(
      now, ++msgcnt, 0,
      add_order_message{{add_order_message::message_type, 0, 0,
                         timestamp{std::chrono::nanoseconds(0)}},
                        ++id,
                        SELL,
                        300,
                        stock,
                        p12});
}

/**
 * Test compute book increase coverage.
 * @tparam based_order_book type used by compute book and order book
 */
template <typename based_order_book>
void test_compute_book_increase_coverage() {
  using book_type = order_book<based_order_book>;
  using compute_type = compute_book<based_order_book>;

  auto cb =
      [](jb::itch5::message_header const&, book_type const& b,
         book_update const& update) {};
  typename compute_type::callback_type const tmp(cb);
  typename based_order_book::config cfg;
  book_type book(cfg);
  compute_type tested(tmp, cfg);

  auto symbols = tested.symbols();
  BOOST_REQUIRE_EQUAL(symbols.size(), std::size_t(0));

  // ... verify compute_book can ignore unknown messages ...
  time_point now = tested.now();
  auto buf = jb::itch5::testing::create_message(
      u'a', timestamp{std::chrono::nanoseconds(10)}, 128);
  tested.handle_unknown(
      now, unknown_message{std::uint32_t(123), std::size_t(1000000), buf.size(),
                           &buf[0]});

  stock_t const stock("HSART");
  price4_t const p10(100000);
  long msgcnt = 100;
  std::uint64_t id = 2;

  // ... verify compute_book can ignore messages irrelevant for book
  // building ...
  tested.handle_message(
      now, ++msgcnt, 0, trade_message{{trade_message::message_type, 0, 0,
                                       timestamp{std::chrono::nanoseconds(0)}},
                                      ++id,
                                      BUY,
                                      100,
                                      stock,
                                      p10,
                                      ++id});
}

/**
 * Test compute book edge cases.
 * @tparam based_order_book Type used by compute_book and order_book
 */
template <typename based_order_book>
void test_compute_book_edge_cases() {
  using book_type = order_book<based_order_book>;
  using compute_type = compute_book<based_order_book>;

  using namespace ::testing;
  ::testing::StrictMock<mock_book_callback> callback;
  auto cb = [&callback](
      jb::itch5::message_header const&, book_type const& b,
      book_update const& update) {
    callback.exec(
        update, b.best_bid(), b.best_offer(), b.buy_count(), b.sell_count());
  };

  // ... create the unit under test ...
  typename based_order_book::config cfg;
  book_type book(cfg);
  compute_type tested(cb, cfg);

  // ... and a number of helper constants and variables to drive the
  // test ...

  stock_t const stock("HSART");
  price4_t const p10(100000);
  long msgcnt = 0;
  std::uint64_t id = 2;

  // ... add an initial order to the book ...
  time_point now = tested.now();
  using namespace ::testing;
  EXPECT_CALL(callback, exec(_, _, _, _, _)).Times(1);
  tested.handle_message(
      now, ++msgcnt, 0,
      add_order_message{{add_order_message::message_type, 0, 0,
                         timestamp{std::chrono::nanoseconds(0)}},
                        ++id,
                        BUY,
                        100,
                        stock,
                        p10});

  // ... try to send the same order, no changes in expectations ...
  tested.handle_message(
      now, ++msgcnt, 0,
      add_order_message{{add_order_message::message_type, 0, 0,
                         timestamp{std::chrono::nanoseconds(0)}},
                        id,
                        BUY,
                        100,
                        stock,
                        p10});
}

/**
 * Test compute book reduction edge cases.
 * @tparam based_order_book Type used by compute_book and order_book
 */
template <typename based_order_book>
void test_compute_book_reduction_edge_cases() {
  using book_type = order_book<based_order_book>;
  using compute_type = compute_book<based_order_book>;

  using namespace ::testing;
  ::testing::StrictMock<mock_book_callback> callback;
  auto cb = [&callback](
      jb::itch5::message_header const&, book_type const& b,
      book_update const& update) {
    callback.exec(
        update, b.best_bid(), b.best_offer(), b.buy_count(), b.sell_count());
  };

  // ... create the unit under test ...
  typename based_order_book::config cfg;
  book_type book(cfg);
  compute_type tested(cb, cfg);

  // ... and a number of helper constants and variables to drive the
  // test ...
  stock_t const stock("HSART");
  price4_t const p10(100000);
  long msgcnt = 0;
  std::uint64_t id = 2;
  auto const id_buy = ++id;

  // ... add an initial order to the book, we expect a callback from
  // that event ...
  using namespace ::testing;
  EXPECT_CALL(callback, exec(_, _, _, _, _)).Times(1);
  time_point now = tested.now();
  tested.handle_message(
      now, ++msgcnt, 0,
      add_order_message{{add_order_message::message_type, 0, 0,
                         timestamp{std::chrono::nanoseconds(0)}},
                        id_buy,
                        BUY,
                        100,
                        stock,
                        p10});

  // ... try to cancel a different order, we expect that no further
  // callbacks are created ...
  now = tested.now();
  tested.handle_message(
      now, ++msgcnt, 0,
      order_cancel_message{{order_cancel_message::message_type, 0, 0,
                            timestamp{std::chrono::nanoseconds(0)}},
                           std::uint64_t(1),
                           100});

  // ... fully cancel the order, we expect a new callback ...
  now = tested.now();
  EXPECT_CALL(callback, exec(_, _, _, _, _)).Times(1);
  tested.handle_message(
      now, ++msgcnt, 0,
      order_delete_message{{order_delete_message::message_type, 0, 0,
                            timestamp{std::chrono::nanoseconds(0)}},
                           id_buy});

  // ... fully cancel the order a second time should produce no action ...
  now = tested.now();
  tested.handle_message(
      now, ++msgcnt, 0,
      order_delete_message{{order_delete_message::message_type, 0, 0,
                            timestamp{std::chrono::nanoseconds(0)}},
                           id_buy});

  // ... add another order to the book, we expect a callback from that
  // event ...
  EXPECT_CALL(callback, exec(_, _, _, _, _)).Times(1);
  auto const id_sell = ++id;
  now = tested.now();
  tested.handle_message(
      now, ++msgcnt, 0,
      add_order_message{{add_order_message::message_type, 0, 0,
                         timestamp{std::chrono::nanoseconds(0)}},
                        id_sell,
                        SELL,
                        300,
                        stock,
                        p10});

  // ... try to cancel more shares than are available in the order ...
  now = tested.now();
  EXPECT_CALL(
      callback, exec(
                    book_update{now, stock, SELL, p10, -300}, empty_bid(),
                    empty_offer(), 0, 0))
      .Times(1);
  tested.handle_message(
      now, ++msgcnt, 0,
      order_cancel_message{{order_cancel_message::message_type, 0, 0,
                            timestamp{std::chrono::nanoseconds(0)}},
                           id_sell,
                           600});
}

/**
 * Test compute book replace edge cases.
 * @tparam based_order_book Type used by compute_book and order_book
 */
template <typename based_order_book>
void test_compute_book_replace_edge_cases() {
  using book_type = order_book<based_order_book>;
  using compute_type = compute_book<based_order_book>;

  ::testing::StrictMock<mock_book_callback> callback;
  auto cb = [&callback](
      jb::itch5::message_header const&, book_type const& b,
      book_update const& update) {
    callback.exec(
        update, b.best_bid(), b.best_offer(), b.buy_count(), b.sell_count());
  };
  // ... create the object under test ...
  typename based_order_book::config cfg;
  book_type book(cfg);
  compute_type tested(cb, cfg);

  // ... and a number of helper constants and variables to drive the
  // test ...
  stock_t const stock("HSART");
  price4_t const p10(100000);
  price4_t const p11(110000);
  long msgcnt = 0;
  std::uint64_t id = 2;
  auto const id_buy_1 = ++id;
  auto const id_buy_2 = ++id;
  auto const id_buy_3 = ++id;

  // ... add an orders to the book ...
  time_point now = tested.now();
  using namespace ::testing;
  EXPECT_CALL(callback, exec(_, _, _, _, _)).Times(1);
  tested.handle_message(
      now, ++msgcnt, 0,
      add_order_message{{add_order_message::message_type, 0, 0,
                         timestamp{std::chrono::nanoseconds(0)}},
                        id_buy_1,
                        BUY,
                        100,
                        stock,
                        p10});

  // ... replacing a non-existing order should have no effect ...
  now = tested.now();
  tested.handle_message(
      now, ++msgcnt, 0,
      order_replace_message{{order_replace_message::message_type, 0, 0,
                             timestamp{std::chrono::nanoseconds(0)}},
                            id_buy_2,
                            id_buy_3,
                            400,
                            p11});

  // ... add a second order, we expect a callback from that event ...
  now = tested.now();
  EXPECT_CALL(callback, exec(_, _, _, _, _)).Times(1);
  tested.handle_message(
      now, ++msgcnt, 0,
      add_order_message{{add_order_message::message_type, 0, 0,
                         timestamp{std::chrono::nanoseconds(0)}},
                        id_buy_2,
                        BUY,
                        300,
                        stock,
                        p11});

  // ... replacing with a duplicate id should have no effect ...
  now = tested.now();
  tested.handle_message(
      now, ++msgcnt, 0,
      order_replace_message{{order_replace_message::message_type, 0, 0,
                             timestamp{std::chrono::nanoseconds(0)}},
                            id_buy_1,
                            id_buy_2,
                            400,
                            p11});
}

/**
 * Test compute book order execute message
 * @tparam based_order_book Type used by compute_book and order_book
 */
template <typename based_order_book>
void test_compute_book_order_executed_message() {
  using book_type = order_book<based_order_book>;
  using compute_type = compute_book<based_order_book>;

  using namespace ::testing;
  ::testing::StrictMock<mock_book_callback> callback;
  auto cb = [&callback](
      jb::itch5::message_header const&, book_type const& b,
      book_update const& update) {
    callback.exec(
        update, b.best_bid(), b.best_offer(), b.buy_count(), b.sell_count());
  };
  // ... create the object under test ...
  typename based_order_book::config cfg;
  book_type book(cfg);
  compute_type tested(cb, cfg);

  // ... and a number of helper constants and variables to drive the
  // test ...
  stock_t const stock("HSART");
  price4_t const p10(100000);
  price4_t const p11(110000);
  long msgcnt = 0;
  std::uint64_t id = 2;
  auto const id_buy = ++id;

  // ... add an initial order to the book, we expect a single update,
  // and the order should be in the book when the update is called ...
  time_point now = tested.now();
  EXPECT_CALL(
      callback, exec(
                    book_update{now, stock, BUY, p10, 500},
                    half_quote{p10, 500}, empty_offer(), 1, 0))
      .Times(1);
  tested.handle_message(
      now, ++msgcnt, 0,
      add_order_mpid_message{{{add_order_mpid_message::message_type, 0, 0,
                               timestamp{std::chrono::nanoseconds(0)}},
                              id_buy,
                              BUY,
                              500,
                              stock,
                              p10},
                             mpid_t("LOOF")});
  // ... we expect the new order to implicitly add the symbol ...
  auto symbols = tested.symbols();
  BOOST_REQUIRE_EQUAL(symbols.size(), std::size_t(1));
  BOOST_CHECK_EQUAL(symbols[0], stock);

  // ... add an order to the opposite side of the book ...
  now = tested.now();
  auto const id_sell = ++id;
  // ... we also expect a single update, and the order should be in
  // the book when the update is called ...
  EXPECT_CALL(
      callback, exec(
                    book_update{now, stock, SELL, p11, 500},
                    half_quote{p10, 500}, half_quote{p11, 500}, 1, 1))
      .Times(1);
  tested.handle_message(
      now, ++msgcnt, 0,
      add_order_mpid_message{{{add_order_mpid_message::message_type, 0, 0,
                               timestamp{std::chrono::nanoseconds(0)}},
                              id_sell,
                              SELL,
                              500,
                              stock,
                              p11},
                             mpid_t("LOOF")});

  // ... execute the BUY order ...
  now = tested.now();
  EXPECT_CALL(
      callback, exec(
                    book_update{now, stock, BUY, p10, -100},
                    half_quote{p10, 400}, half_quote{p11, 500}, 1, 1))
      .Times(1);
  tested.handle_message(
      now, ++msgcnt, 0,
      order_executed_message{{order_executed_message::message_type, 0, 0,
                              timestamp{std::chrono::nanoseconds(0)}},
                             id_buy,
                             100,
                             ++id});

  // ... execute the SELL order ...
  now = tested.now();
  EXPECT_CALL(
      callback, exec(
                    book_update{now, stock, SELL, p11, -100},
                    half_quote{p10, 400}, half_quote{p11, 400}, 1, 1))
      .Times(1);
  tested.handle_message(
      now, ++msgcnt, 0,
      order_executed_message{{order_executed_message::message_type, 0, 0,
                              timestamp{std::chrono::nanoseconds(0)}},
                             id_sell,
                             100,
                             ++id});

  // ... execute the BUY order with a price ...
  now = tested.now();
  EXPECT_CALL(
      callback, exec(
                    book_update{now, stock, BUY, p10, -100},
                    half_quote{p10, 300}, half_quote{p11, 400}, 1, 1))
      .Times(1);
  tested.handle_message(
      now, ++msgcnt, 0, order_executed_price_message{
                            {{order_executed_price_message::message_type, 0, 0,
                              timestamp{std::chrono::nanoseconds(0)}},
                             id_buy,
                             100,
                             ++id},
                            printable_t('N'),
                            price4_t(99901)});

  // ... execute the SELL order with a price ...
  now = tested.now();
  EXPECT_CALL(
      callback, exec(
                    book_update{now, stock, SELL, p11, -100},
                    half_quote{p10, 300}, half_quote{p11, 300}, 1, 1))
      .Times(1);
  tested.handle_message(
      now, ++msgcnt, 0, order_executed_price_message{
                            {{order_executed_price_message::message_type, 0, 0,
                              timestamp{std::chrono::nanoseconds(0)}},
                             id_sell,
                             100,
                             ++id},
                            printable_t('N'),
                            price4_t(110001)});

  // ... complete the execution of the BUY order ...
  now = tested.now();
  EXPECT_CALL(
      callback, exec(
                    book_update{now, stock, BUY, p10, -300}, empty_bid(),
                    half_quote{p11, 300}, 0, 1))
      .Times(1);
  tested.handle_message(
      now, ++msgcnt, 0,
      order_executed_message{{order_executed_message::message_type, 0, 0,
                              timestamp{std::chrono::nanoseconds(0)}},
                             id_buy,
                             300,
                             ++id});

  // ... execute the SELL order with a price ...
  now = tested.now();
  EXPECT_CALL(
      callback, exec(
                    book_update{now, stock, SELL, p11, -300}, empty_bid(),
                    empty_offer(), 0, 0))
      .Times(1);
  tested.handle_message(
      now, ++msgcnt, 0,
      order_executed_message{{order_executed_message::message_type, 0, 0,
                              timestamp{std::chrono::nanoseconds(0)}},
                             id_sell,
                             300,
                             ++id});
}

/**
 * Test compute book order replace message.
 * @tparam based_order_book Type used by compute_book and order_book
 */
template <typename based_order_book>
void test_compute_book_order_replace_message() {
  using book_type = order_book<based_order_book>;
  using compute_type = compute_book<based_order_book>;

  using namespace ::testing;
  ::testing::StrictMock<mock_book_callback> callback;
  auto cb = [&callback](
      jb::itch5::message_header const&, book_type const& b,
      book_update const& update) {
    callback.exec(
        update, b.best_bid(), b.best_offer(), b.buy_count(), b.sell_count());
  };
  // ... create the object under test ...
  typename based_order_book::config cfg;
  book_type book(cfg);
  compute_type tested(cb, cfg);

  // ... and a number of helper constants and variables to drive the
  // test ...
  stock_t const stock("HSART");
  price4_t const p10(100000);
  price4_t const p11(110000);
  price4_t const p12(120000);
  long msgcnt = 0;
  std::uint64_t id = 2;
  auto const id_buy = ++id;

  // ... add an initial order to the book, we expect a single update,
  // and the order should be in the book when the update is called ...
  time_point now = tested.now();
  EXPECT_CALL(
      callback, exec(
                    book_update{now, stock, BUY, p10, 500},
                    half_quote{p10, 500}, empty_offer(), 1, 0))
      .Times(1);
  tested.handle_message(
      now, ++msgcnt, 0,
      add_order_mpid_message{{{add_order_mpid_message::message_type, 0, 0,
                               timestamp{std::chrono::nanoseconds(0)}},
                              id_buy,
                              BUY,
                              500,
                              stock,
                              p10},
                             mpid_t("LOOF")});
  // ... we also expect the new order to implicitly add the symbol ...
  auto symbols = tested.symbols();
  BOOST_REQUIRE_EQUAL(symbols.size(), std::size_t(1));
  BOOST_CHECK_EQUAL(symbols[0], stock);

  // ... add an order to the opposite side of the book, we expect a
  // single update, and the order should be in the book when the
  // update is called ...
  now = tested.now();
  EXPECT_CALL(
      callback, exec(
                    book_update{now, stock, SELL, p11, 500},
                    half_quote{p10, 500}, half_quote{p11, 500}, 1, 1))
      .Times(1);
  auto const id_sell = ++id;
  tested.handle_message(
      now, ++msgcnt, 0,
      add_order_mpid_message{{{add_order_mpid_message::message_type, 0, 0,
                               timestamp{std::chrono::nanoseconds(0)}},
                              id_sell,
                              SELL,
                              500,
                              stock,
                              p11},
                             mpid_t("LOOF")});

  // ... replace the SELL order ...
  now = tested.now();
  auto id_sell_replx = ++id;
  EXPECT_CALL(
      callback, exec(
                    book_update{now, stock, SELL, p12, 600, true, p11, 500},
                    half_quote{p10, 500}, half_quote{p12, 600}, 1, 1))
      .Times(1);
  tested.handle_message(
      now, ++msgcnt, 0,
      order_replace_message{{order_replace_message::message_type, 0, 0,
                             timestamp{std::chrono::nanoseconds(0)}},
                            id_sell,
                            id_sell_replx,
                            600,
                            p12});

  // ... replace the BUY order ...
  now = tested.now();
  auto id_buy_replx = ++id;
  EXPECT_CALL(
      callback, exec(
                    book_update{now, stock, BUY, p11, 600, true, p12, 500},
                    half_quote{p11, 600}, half_quote{p12, 600}, 1, 1))
      .Times(1);
  tested.handle_message(
      now, ++msgcnt, 0,
      order_replace_message{{order_replace_message::message_type, 0, 0,
                             timestamp{std::chrono::nanoseconds(0)}},
                            id_buy,
                            id_buy_replx,
                            600,
                            p11});
}

/**
 * Test compute book order cancel message.
 * @tparam based_order_book Type used by compute_book and order_book
 */
template <typename based_order_book>
void test_compute_book_order_cancel_message() {
  using book_type = order_book<based_order_book>;
  using compute_type = compute_book<based_order_book>;

  using namespace ::testing;
  ::testing::StrictMock<mock_book_callback> callback;
  auto cb = [&callback](
      jb::itch5::message_header const&, book_type const& b,
      book_update const& update) {
    callback.exec(
        update, b.best_bid(), b.best_offer(), b.buy_count(), b.sell_count());
  };

  typename based_order_book::config cfg;
  compute_type tested(cb, cfg);

  // ... and a number of helper constants and variables to drive the
  // test ...
  stock_t const stock("HSART");
  price4_t const p10(100000);
  price4_t const p11(110000);
  long msgcnt = 0;
  std::uint64_t id = 2;
  auto const id_buy = ++id;

  // ... add an initial order to the book, we expect a single update,
  // and the order should be in the book when the update is called ...
  time_point now = tested.now();
  EXPECT_CALL(
      callback, exec(
                    book_update{now, stock, BUY, p10, 500},
                    half_quote{p10, 500}, empty_offer(), 1, 0))
      .Times(1);
  tested.handle_message(
      now, ++msgcnt, 0,
      add_order_mpid_message{{{add_order_mpid_message::message_type, 0, 0,
                               timestamp{std::chrono::nanoseconds(0)}},
                              id_buy,
                              BUY,
                              500,
                              stock,
                              p10},
                             mpid_t("LOOF")});
  // ... we expect the new order to implicitly add the symbol ...
  auto symbols = tested.symbols();
  BOOST_REQUIRE_EQUAL(symbols.size(), std::size_t(1));
  BOOST_CHECK_EQUAL(symbols[0], stock);

  // ... add an order to the opposite side of the book, we expect a
  // single update, and the order should be in the book when the
  // update is called ...
  now = tested.now();
  auto const id_sell = ++id;
  EXPECT_CALL(
      callback, exec(
                    book_update{now, stock, SELL, p11, 500},
                    half_quote{p10, 500}, half_quote{p11, 500}, 1, 1))
      .Times(1);
  tested.handle_message(
      now, ++msgcnt, 0,
      add_order_mpid_message{{{add_order_mpid_message::message_type, 0, 0,
                               timestamp{std::chrono::nanoseconds(0)}},
                              id_sell,
                              SELL,
                              500,
                              stock,
                              p11},
                             mpid_t("LOOF")});

  // ... cancel 100 shares in the BUY order ...
  now = tested.now();
  EXPECT_CALL(
      callback, exec(
                    book_update{now, stock, BUY, p10, -100},
                    half_quote{p10, 400}, half_quote{p11, 500}, 1, 1))
      .Times(1);
  tested.handle_message(
      now, ++msgcnt, 0,
      order_cancel_message{{order_cancel_message::message_type, 0, 0,
                            timestamp{std::chrono::nanoseconds(0)}},
                           id_buy,
                           100});

  // ... cancel 100 shares in the SELL order ...
  now = tested.now();
  EXPECT_CALL(
      callback, exec(
                    book_update{now, stock, SELL, p11, -100},
                    half_quote{p10, 400}, half_quote{p11, 400}, 1, 1))
      .Times(1);
  tested.handle_message(
      now, ++msgcnt, 0,
      order_cancel_message{{order_cancel_message::message_type, 0, 0,
                            timestamp{std::chrono::nanoseconds(0)}},
                           id_sell,
                           100});

  // ... fully cancel the BUY order ...
  now = tested.now();
  EXPECT_CALL(
      callback, exec(
                    book_update{now, stock, BUY, p10, -400}, empty_bid(),
                    half_quote{p11, 400}, 0, 1))
      .Times(1);
  tested.handle_message(
      now, ++msgcnt, 0,
      order_delete_message{{order_delete_message::message_type, 0, 0,
                            timestamp{std::chrono::nanoseconds(0)}},
                           id_buy});

  // ... fully cancel the SELL order ...
  now = tested.now();
  EXPECT_CALL(
      callback, exec(
                    book_update{now, stock, SELL, p11, -400}, empty_bid(),
                    empty_offer(), 0, 0))
      .Times(1);
  tested.handle_message(
      now, ++msgcnt, 0,
      order_delete_message{{order_delete_message::message_type, 0, 0,
                            timestamp{std::chrono::nanoseconds(0)}},
                           id_sell});
}

/**
 * Test compute book stock directory message.
 * @tparam based_order_book Type used by compute_book and order_book
 */
template <typename based_order_book>
void test_compute_book_stock_directory_message() {
  using book_type = order_book<based_order_book>;
  using compute_type = compute_book<based_order_book>;
  auto cb =
      [](jb::itch5::message_header const&, book_type const&,
         book_update const& update) {};

  typename based_order_book::config cfg;
  book_type book(cfg);
  compute_type tested(cb, cfg);

  time_point now = tested.now();
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

} // namespace testing
} // namesapce itch5
} // namespace jb

/**
 * @test Verify compute book handle_message works as
 * expected for add_order_message.
 */
BOOST_AUTO_TEST_CASE(compute_book_add_order_message) {
  using namespace jb::itch5;
  namespace t = jb::itch5::testing;
  t::test_compute_book_add_order_message_buy<map_based_order_book>();
  t::test_compute_book_add_order_message_sell<map_based_order_book>();

  t::test_compute_book_add_order_message_buy<array_based_order_book>();
  t::test_compute_book_add_order_message_sell<array_based_order_book>();
}

/**
 * @test Increase code coverage in jb::itch5::compute_book
 */
BOOST_AUTO_TEST_CASE(compute_book_increase_coverage) {
  using namespace jb::itch5;
  namespace t = jb::itch5::testing;
  t::test_compute_book_increase_coverage<map_based_order_book>();
  t::test_compute_book_increase_coverage<array_based_order_book>();
}

/**
 * @test Increase code coverage in
 * jb::itch5::compute_book::handle_message for add_order_message.
 */
BOOST_AUTO_TEST_CASE(compute_book_add_order_message_edge_cases) {
  using namespace jb::itch5;
  namespace t = jb::itch5::testing;
  t::test_compute_book_edge_cases<map_based_order_book>();
  t::test_compute_book_edge_cases<array_based_order_book>();
}

/**
 * @test Increase code coverage in
 * jb::itch5::compute_book::handle_order_reduction
 */
BOOST_AUTO_TEST_CASE(compute_book_reduction_edge_cases) {
  using namespace jb::itch5;
  namespace t = jb::itch5::testing;
  t::test_compute_book_reduction_edge_cases<map_based_order_book>();
  t::test_compute_book_reduction_edge_cases<array_based_order_book>();
}

/**
 * @test Increase code coverage for order_replace_message
 */
BOOST_AUTO_TEST_CASE(compute_book_replace_edge_cases) {
  using namespace jb::itch5;
  namespace t = jb::itch5::testing;
  t::test_compute_book_replace_edge_cases<map_based_order_book>();
  t::test_compute_book_replace_edge_cases<array_based_order_book>();
}

/**
 * @test Verify that jb::itch5::compute_book::handle_message works as
 * expected for order_executed_message BUY & SELL.
 */
BOOST_AUTO_TEST_CASE(compute_book_order_executed_message) {
  using namespace jb::itch5;
  namespace t = jb::itch5::testing;
  t::test_compute_book_order_executed_message<map_based_order_book>();
  t::test_compute_book_order_executed_message<array_based_order_book>();
}

/**
 * @test Verify that jb::itch5::compute_book::handle_message works as
 * expected for order_replace_message BUY & SELL.
 */
BOOST_AUTO_TEST_CASE(compute_book_order_replace_message) {
  using namespace jb::itch5;
  namespace t = jb::itch5::testing;
  t::test_compute_book_order_replace_message<map_based_order_book>();
  t::test_compute_book_order_replace_message<array_based_order_book>();
}

/**
 * @test Verify that jb::itch5::compute_book::handle_message works as
 * expected for order_cancel_message and order_delete_message, both sides.
 */
BOOST_AUTO_TEST_CASE(compute_book_order_cancel_message) {
  using namespace jb::itch5;
  namespace t = jb::itch5::testing;
  t::test_compute_book_order_cancel_message<map_based_order_book>();
  t::test_compute_book_order_cancel_message<array_based_order_book>();
}

/**
 * @test Verify that jb::itch5::compute_book::handle_message works as
 *expected for stock_directory_message.
 */
BOOST_AUTO_TEST_CASE(compute_book_stock_directory_message) {
  using namespace jb::itch5;
  namespace t = jb::itch5::testing;
  t::test_compute_book_stock_directory_message<map_based_order_book>();
  t::test_compute_book_stock_directory_message<array_based_order_book>();
}

/**
 * @test Verify that jb::itch5::book_update operators
 * work as expected.
 */
BOOST_AUTO_TEST_CASE(compute_book_book_update_operators) {
  using namespace jb::itch5;
  namespace t = jb::itch5::testing;

  auto const ts0 = clock_type::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  auto const ts1 = clock_type::now();

  BOOST_CHECK_EQUAL(
      book_update({ts0, stock_t("A"), t::BUY, price4_t(1000), 100}),
      book_update({ts0, stock_t("A"), t::BUY, price4_t(1000), 100}));
  BOOST_CHECK_NE(
      book_update({ts0, stock_t("A"), t::BUY, price4_t(1000), 100}),
      book_update({ts1, stock_t("A"), t::BUY, price4_t(1000), 100}));
  BOOST_CHECK_NE(
      book_update({ts0, stock_t("A"), t::BUY, price4_t(1000), 100}),
      book_update({ts0, stock_t("B"), t::BUY, price4_t(1000), 100}));
  BOOST_CHECK_NE(
      book_update({ts0, stock_t("A"), t::BUY, price4_t(1000), 10}),
      book_update({ts0, stock_t("A"), t::SELL, price4_t(1000), 10}));
  BOOST_CHECK_NE(
      book_update({ts0, stock_t("A"), t::BUY, price4_t(1000), 100}),
      book_update({ts0, stock_t("A"), t::BUY, price4_t(1001), 100}));
  BOOST_CHECK_NE(
      book_update({ts0, stock_t("A"), t::BUY, price4_t(1000), 100}),
      book_update({ts0, stock_t("B"), t::BUY, price4_t(1000), 200}));

  std::ostringstream os;
  os << book_update{ts1, stock_t("A"), t::BUY, price4_t(1000), 300};
  BOOST_CHECK_EQUAL(os.str(), "{A,B,0.1000,300}");
}

