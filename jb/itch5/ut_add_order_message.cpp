#include <jb/itch5/add_order_message.hpp>
#include <jb/itch5/testing_data.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that the jb::itch5::add_order_message decoder works
 * as expected.
 */
BOOST_AUTO_TEST_CASE(decode_add_order_message) {
  using namespace jb::itch5;
  using namespace std::chrono;

  auto buf = jb::itch5::testing::add_order();
  auto expected_ts = jb::itch5::testing::expected_ts();

  auto x = decoder<true, add_order_message>::r(buf.second, buf.first, 0);
  BOOST_CHECK_EQUAL(x.header.message_type, add_order_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.order_reference_number, 42ULL);
  BOOST_CHECK_EQUAL(x.buy_sell_indicator, buy_sell_indicator_t(u'B'));
  BOOST_CHECK_EQUAL(x.shares, 100);
  BOOST_CHECK_EQUAL(x.stock, "HSART");
  BOOST_CHECK_EQUAL(x.price, price4_t(1230500));

  x = decoder<false, add_order_message>::r(buf.second, buf.first, 0);
  BOOST_CHECK_EQUAL(x.header.message_type, add_order_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.order_reference_number, 42ULL);
  BOOST_CHECK_EQUAL(x.buy_sell_indicator, buy_sell_indicator_t(u'B'));
  BOOST_CHECK_EQUAL(x.shares, 100);
  BOOST_CHECK_EQUAL(x.stock, "HSART");
  BOOST_CHECK_EQUAL(x.price, price4_t(1230500));
}

/**
 * @test Verify that jb::itch5::add_order_message iostream
 * operator works as expected.
 */
BOOST_AUTO_TEST_CASE(stream_add_order_message) {
  using namespace std::chrono;
  using namespace jb::itch5;

  auto buf = jb::itch5::testing::add_order();
  auto tmp = decoder<false, add_order_message>::r(buf.second, buf.first, 0);
  std::ostringstream os;
  os << tmp;
  BOOST_CHECK_EQUAL(os.str(), "message_type=A,stock_locate=0"
                              ",tracking_number=1,timestamp=113231.123456789"
                              ",order_reference_number=42"
                              ",buy_sell_indicator=B"
                              ",shares=100"
                              ",stock=HSART"
                              ",price=123.0500");
}

/**
 * @test Verify that buy_sell_indicator_t works as expected.
 */
BOOST_AUTO_TEST_CASE(simple_buy_sell_indicator) {
  using namespace jb::itch5;
  BOOST_CHECK_NO_THROW(buy_sell_indicator_t(u'B'));
  BOOST_CHECK_NO_THROW(buy_sell_indicator_t(u'S'));
  BOOST_CHECK_THROW(buy_sell_indicator_t(u'*'), std::runtime_error);
}
