#include "jb/itch5/compute_inside.hpp"
#include <jb/itch5/testing/messages.hpp>
#include <jb/as_hhmmss.hpp>

#include <skye/mock_function.hpp>
#include <boost/test/unit_test.hpp>

using namespace jb::itch5;

namespace std { // oh the horror

bool operator==(
    jb::itch5::half_quote const& lhs, jb::itch5::half_quote const& rhs) {
  return lhs.first == rhs.first and lhs.second == rhs.second;
}

std::ostream& operator<<(std::ostream& os, jb::itch5::half_quote const& x) {
  return os << "{" << x.first << "," << x.second << "}";
}

} // namespace std

namespace {

buy_sell_indicator_t const BUY(u'B');
buy_sell_indicator_t const SELL(u'S');

/// Create a simple timestamp
timestamp create_timestamp() {
  return timestamp{std::chrono::nanoseconds(0)};
}
} // anonymous namespace

/**
 * @test Verify that jb::itch5::order_book works as expected.
 */
BOOST_AUTO_TEST_CASE(compute_inside_simple) {
  // We are going to use a mock function to handle the callback
  // because it is easy to test what values they got ...
  skye::mock_function<void(
      compute_inside::time_point, stock_t, half_quote const&,
      half_quote const&)>
      callback;

  // ... create a callback that holds a reference to the mock
  // function, because the handler keeps the callback by value.  Also,
  // ignore the header, because it is tedious to test for it ...
  auto cb = [&callback](
      compute_inside::time_point recv_ts, message_header const&,
      stock_t const& stock, half_quote const& bid,
      half_quote const& offer) { callback(recv_ts, stock, bid, offer); };
  // ... create the object under testing ...
  compute_inside tested(cb);

  // ... we do not expect any callbacks ...
  callback.check_called().never();

  // ... send a couple of stock directory messages, do not much care
  // about their contents other than the symbol ...
  using namespace jb::itch5::testing;
  compute_inside::time_point now = tested.now();
  long msgcnt = 0;
  tested.handle_message(now, ++msgcnt, 0, create_stock_directory("HSART"));
  tested.handle_message(now, ++msgcnt, 0, create_stock_directory("FOO"));
  tested.handle_message(now, ++msgcnt, 0, create_stock_directory("BAR"));
  // ... duplicates should not create a problem ...
  tested.handle_message(now, ++msgcnt, 0, create_stock_directory("HSART"));
  callback.check_called().never();

  // ... handle a new order ...
  now = tested.now();
  tested.handle_message(
      now, ++msgcnt, 0, add_order_message{{add_order_message::message_type, 0,
                                           0, create_timestamp()},
                                          2,
                                          BUY,
                                          100,
                                          stock_t("HSART"),
                                          price4_t(100000)});
  callback.check_called().once().with(
      compute_inside::time_point(now), stock_t("HSART"),
      half_quote(price4_t(100000), 100), order_book::empty_offer());

  // ... handle a new order on the opposite side of the book ...
  now = tested.now();
  tested.handle_message(
      now, ++msgcnt, 0, add_order_message{{add_order_message::message_type, 0,
                                           0, create_timestamp()},
                                          3,
                                          SELL,
                                          100,
                                          stock_t("HSART"),
                                          price4_t(100100)});
  callback.check_called().once().with(
      compute_inside::time_point(now), stock_t("HSART"),
      half_quote(price4_t(100000), 100), half_quote(price4_t(100100), 100));

  // ... handle a new order with an mpid ...
  now = tested.now();
  tested.handle_message(
      now, ++msgcnt, 0,
      add_order_mpid_message{
          add_order_message{
              {add_order_mpid_message::message_type, 0, 0, create_timestamp()},
              4,
              SELL,
              500,
              stock_t("HSART"),
              price4_t(100100)},
          mpid_t("LOOF")});
  // ... updates the inside just like a regular order ...
  callback.check_called().once().with(
      compute_inside::time_point(now), stock_t("HSART"),
      half_quote(price4_t(100000), 100), half_quote(price4_t(100100), 600));

  // ... handle a partial execution ...
  now = tested.now();
  tested.handle_message(
      now, ++msgcnt, 0,
      order_executed_message{
          {add_order_mpid_message::message_type, 0, 0, create_timestamp()},
          4,
          100,
          123456});
  callback.check_called().once().with(
      compute_inside::time_point(now), stock_t("HSART"),
      half_quote(price4_t(100000), 100), half_quote(price4_t(100100), 500));

  // ... handle a full execution ...
  now = tested.now();
  tested.handle_message(
      now, ++msgcnt, 0,
      order_executed_message{
          {add_order_mpid_message::message_type, 0, 0, create_timestamp()},
          3,
          100,
          123457});
  callback.check_called().once().with(
      compute_inside::time_point(now), stock_t("HSART"),
      half_quote(price4_t(100000), 100), half_quote(price4_t(100100), 400));
  BOOST_CHECK_EQUAL(tested.live_order_count(), 2);

  // ... handle a partial execution with price ...
  now = tested.now();
  tested.handle_message(
      now, ++msgcnt, 0,
      order_executed_price_message{
          order_executed_message{{order_executed_price_message::message_type, 0,
                                  0, create_timestamp()},
                                 4,
                                 100,
                                 123456},
          printable_t{u'Y'}, price4_t(100150)});
  callback.check_called().once().with(
      compute_inside::time_point(now), stock_t("HSART"),
      half_quote(price4_t(100000), 100), half_quote(price4_t(100100), 300));
  BOOST_CHECK_EQUAL(tested.live_order_count(), 2);

  // ... create yet another order ...
  now = tested.now();
  tested.handle_message(
      now, ++msgcnt, 0, add_order_message{{add_order_message::message_type, 0,
                                           0, create_timestamp()},
                                          5,
                                          BUY,
                                          1000,
                                          stock_t("HSART"),
                                          price4_t(100000)});
  callback.check_called().once().with(
      compute_inside::time_point(now), stock_t("HSART"),
      half_quote(price4_t(100000), 1100), half_quote(price4_t(100100), 300));
  BOOST_CHECK_EQUAL(tested.live_order_count(), 3);

  // ... partially cancel the order ...
  now = tested.now();
  tested.handle_message(
      now, ++msgcnt, 0,
      order_cancel_message{
          {order_cancel_message::message_type, 0, 0, create_timestamp()},
          5,
          200});
  callback.check_called().once().with(
      compute_inside::time_point(now), stock_t("HSART"),
      half_quote(price4_t(100000), 900), half_quote(price4_t(100100), 300));

  // ... fully cancel the order ...
  now = tested.now();
  tested.handle_message(
      now, ++msgcnt, 0,
      order_delete_message{
          {order_delete_message::message_type, 0, 0, create_timestamp()}, 5});
  callback.check_called().once().with(
      compute_inside::time_point(now), stock_t("HSART"),
      half_quote(price4_t(100000), 100), half_quote(price4_t(100100), 300));
}

/**
 * @test Verify that jb::itch5::order_book works as expected for replace.
 *
 * Order replaces have several scenarios, the previous test was
 * getting too big.
 */
BOOST_AUTO_TEST_CASE(compute_inside_replace) {
  // We are going to use a mock function to handle the callback
  // because it is easy to test what values they got ...
  skye::mock_function<void(
      compute_inside::time_point, stock_t, half_quote const&,
      half_quote const&)>
      callback;

  // ... create a callback that holds a reference to the mock
  // function, because the handler keeps the callback by value.  Also,
  // ignore the header, because it is tedious to test for it ...
  auto cb = [&callback](
      compute_inside::time_point recv_ts, message_header const&,
      stock_t const& stock, half_quote const& bid,
      half_quote const& offer) { callback(recv_ts, stock, bid, offer); };
  // ... create the object under testing ...
  compute_inside tested(cb);

  // ... setup the book with orders on both sides ...
  auto now = tested.now();
  int msgcnt = 0;
  tested.handle_message(
      now, ++msgcnt, 0, add_order_message{{add_order_message::message_type, 0,
                                           0, create_timestamp()},
                                          1,
                                          BUY,
                                          500,
                                          stock_t("HSART"),
                                          price4_t(100000)});
  callback.check_called().once().with(
      compute_inside::time_point(now), stock_t("HSART"),
      half_quote(price4_t(100000), 500), order_book::empty_offer());
  now = tested.now();
  tested.handle_message(
      now, ++msgcnt, 0, add_order_message{{add_order_message::message_type, 0,
                                           0, create_timestamp()},
                                          2,
                                          SELL,
                                          500,
                                          stock_t("HSART"),
                                          price4_t(100500)});
  callback.check_called().once().with(
      compute_inside::time_point(now), stock_t("HSART"),
      half_quote(price4_t(100000), 500), half_quote(price4_t(100500), 500));

  // ... handle a replace message that improves the price ...
  now = tested.now();
  tested.handle_message(
      now, ++msgcnt, 0,
      order_replace_message{
          {order_replace_message::message_type, 0, 0, create_timestamp()},
          1,
          3,
          600,
          price4_t(100100)});
  callback.check_called().once().with(
      compute_inside::time_point(now), stock_t("HSART"),
      half_quote(price4_t(100100), 600), half_quote(price4_t(100500), 500));

  // ... handle a replace that changes the qty ...
  now = tested.now();
  tested.handle_message(
      now, ++msgcnt, 0,
      order_replace_message{
          {order_replace_message::message_type, 0, 0, create_timestamp()},
          3,
          4,
          300,
          price4_t(100100)});
  callback.check_called().once().with(
      compute_inside::time_point(now), stock_t("HSART"),
      half_quote(price4_t(100100), 300), half_quote(price4_t(100500), 500));

  // ... handle a replace that changes lowers the best price ...
  now = tested.now();
  tested.handle_message(
      now, ++msgcnt, 0,
      order_replace_message{
          {order_replace_message::message_type, 0, 0, create_timestamp()},
          4,
          9,
          400,
          price4_t(99900)});
  callback.check_called().once().with(
      compute_inside::time_point(now), stock_t("HSART"),
      half_quote(price4_t(99900), 400), half_quote(price4_t(100500), 500));
}

/**
 * @test Improve code coverage for edge cases.
 */
BOOST_AUTO_TEST_CASE(compute_inside_edge_cases) {
  // We are going to use a mock function to handle the callback
  // because it is easy to test what values they got ...
  skye::mock_function<void(
      compute_inside::time_point, stock_t, half_quote const&,
      half_quote const&)>
      callback;

  // ... create a callback that holds a reference to the mock
  // function, because the handler keeps the callback by value.  Also,
  // ignore the header, because it is tedious to test for it ...
  auto cb = [&callback](
      compute_inside::time_point recv_ts, message_header const&,
      stock_t const& stock, half_quote const& bid,
      half_quote const& offer) { callback(recv_ts, stock, bid, offer); };
  // ... create the object under testing ...
  compute_inside tested(cb);

  // ... force an execution on a non-existing order ...
  auto now = tested.now();
  int msgcnt = 0;
  tested.handle_message(
      now, ++msgcnt, 0,
      order_executed_message{
          {add_order_mpid_message::message_type, 0, 0, create_timestamp()},
          4,
          100,
          123456});
  callback.check_called().never();

  // ... improve code coverage for unknown messages ...
  now = tested.now();
  char const unknownbuf[] = "foobarbaz";
  tested.handle_unknown(
      now, jb::itch5::unknown_message(
               ++msgcnt, 0, sizeof(unknownbuf) - 1, unknownbuf));

  // ... a completely new symbol might be slow, but should work ...
  now = tested.now();
  tested.handle_message(
      now, ++msgcnt, 0, add_order_message{{add_order_message::message_type, 0,
                                           0, create_timestamp()},
                                          1,
                                          BUY,
                                          500,
                                          stock_t("CRAZY"),
                                          price4_t(150000)});
  callback.check_called().once().with(
      compute_inside::time_point(now), stock_t("CRAZY"),
      half_quote(price4_t(150000), 500), order_book::empty_offer());

  // ... remember the previous timestamp because we will use it in a
  // further check ...
  auto previous = now;

  // ... a duplicate order id should result in no changes ...
  now = tested.now();
  tested.handle_message(
      now, ++msgcnt, 0, add_order_message{{add_order_message::message_type, 0,
                                           0, create_timestamp()},
                                          1,
                                          SELL,
                                          700,
                                          stock_t("CRAZY"),
                                          price4_t(160000)});
  // ... no *new* callback is expected, verify that the previous one
  // is the only order there ...
  callback.check_called().with(
      compute_inside::time_point(previous), stock_t("CRAZY"),
      half_quote(price4_t(150000), 500), order_book::empty_offer());
}
