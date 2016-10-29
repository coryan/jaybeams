#include <jb/itch5/cross_trade_message.hpp>
#include <jb/itch5/testing_data.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that the jb::itch5::cross_trade_message decoder works
 * as expected.
 */
BOOST_AUTO_TEST_CASE(decode_cross_trade_message) {
  using namespace jb::itch5;
  using namespace std::chrono;

  auto buf = jb::itch5::testing::cross_trade();
  auto expected_ts = jb::itch5::testing::expected_ts();

  auto x = decoder<true, cross_trade_message>::r(buf.second, buf.first, 0);
  BOOST_CHECK_EQUAL(x.header.message_type, cross_trade_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.shares, 424200ULL);
  BOOST_CHECK_EQUAL(x.stock, "HSART");
  BOOST_CHECK_EQUAL(x.cross_price, price4_t(1230500));
  BOOST_CHECK_EQUAL(x.match_number, 2340600ULL);
  BOOST_CHECK_EQUAL(x.cross_type, cross_type_t(u'H'));

  x = decoder<false, cross_trade_message>::r(buf.second, buf.first, 0);
  BOOST_CHECK_EQUAL(x.header.message_type, cross_trade_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.shares, 424200ULL);
  BOOST_CHECK_EQUAL(x.stock, "HSART");
  BOOST_CHECK_EQUAL(x.cross_price, price4_t(1230500));
  BOOST_CHECK_EQUAL(x.match_number, 2340600ULL);
  BOOST_CHECK_EQUAL(x.cross_type, cross_type_t(u'H'));
}

/**
 * @test Verify that jb::itch5::cross_trade_message iostream
 * operator works as expected.
 */
BOOST_AUTO_TEST_CASE(stream_cross_trade_message) {
  using namespace std::chrono;
  using namespace jb::itch5;

  auto buf = jb::itch5::testing::cross_trade();
  auto tmp = decoder<false, cross_trade_message>::r(buf.second, buf.first, 0);
  std::ostringstream os;
  os << tmp;
  BOOST_CHECK_EQUAL(os.str(), "message_type=Q,stock_locate=0"
                              ",tracking_number=1,timestamp=113231.123456789"
                              ",shares=424200"
                              ",stock=HSART"
                              ",cross_price=123.0500"
                              ",match_number=2340600"
                              ",cross_type=H");
}
