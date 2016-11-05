#include <jb/itch5/net_order_imbalance_indicator_message.hpp>
#include <jb/itch5/testing/data.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that the jb::itch5::net_order_imbalance_indicator_message
 * decoder works
 * as expected.
 */
BOOST_AUTO_TEST_CASE(decode_net_order_imbalance_indicator_message) {
  using namespace jb::itch5;
  using namespace std::chrono;

  auto buf = jb::itch5::testing::net_order_imbalance_indicator();
  auto expected_ts = jb::itch5::testing::expected_ts();

  auto x = decoder<true, net_order_imbalance_indicator_message>::r(
      buf.second, buf.first, 0);
  BOOST_CHECK_EQUAL(
      x.header.message_type,
      net_order_imbalance_indicator_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.paired_shares, 42000000ULL);
  BOOST_CHECK_EQUAL(x.imbalance_shares, 424200ULL);
  BOOST_CHECK_EQUAL(x.imbalance_direction, imbalance_direction_t(u'B'));
  BOOST_CHECK_EQUAL(x.stock, "HSART");
  BOOST_CHECK_EQUAL(x.far_price, price4_t(2340600));
  BOOST_CHECK_EQUAL(x.near_price, price4_t(1230500));
  BOOST_CHECK_EQUAL(x.current_reference_price, price4_t(890100));
  BOOST_CHECK_EQUAL(x.cross_type, cross_type_t(u'O'));
  BOOST_CHECK_EQUAL(
      x.price_variation_indicator, price_variation_indicator_t(u'A'));

  x = decoder<false, net_order_imbalance_indicator_message>::r(
      buf.second, buf.first, 0);
  BOOST_CHECK_EQUAL(
      x.header.message_type,
      net_order_imbalance_indicator_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.paired_shares, 42000000ULL);
  BOOST_CHECK_EQUAL(x.imbalance_shares, 424200ULL);
  BOOST_CHECK_EQUAL(x.imbalance_direction, imbalance_direction_t(u'B'));
  BOOST_CHECK_EQUAL(x.stock, "HSART");
  BOOST_CHECK_EQUAL(x.far_price, price4_t(2340600));
  BOOST_CHECK_EQUAL(x.near_price, price4_t(1230500));
  BOOST_CHECK_EQUAL(x.current_reference_price, price4_t(890100));
  BOOST_CHECK_EQUAL(x.cross_type, cross_type_t(u'O'));
  BOOST_CHECK_EQUAL(
      x.price_variation_indicator, price_variation_indicator_t(u'A'));
}

/**
 * @test Verify that jb::itch5::net_order_imbalance_indicator_message iostream
 * operator works as expected.
 */
BOOST_AUTO_TEST_CASE(stream_net_order_imbalance_indicator_message) {
  using namespace std::chrono;
  using namespace jb::itch5;

  auto buf = jb::itch5::testing::net_order_imbalance_indicator();
  auto tmp = decoder<false, net_order_imbalance_indicator_message>::r(
      buf.second, buf.first, 0);
  std::ostringstream os;
  os << tmp;
  BOOST_CHECK_EQUAL(
      os.str(), "message_type=I,stock_locate=0"
                ",tracking_number=1,timestamp=113231.123456789"
                ",paired_shares=42000000"
                ",imbalance_shares=424200"
                ",imbalance_direction=B"
                ",stock=HSART"
                ",far_price=234.0600"
                ",near_price=123.0500"
                ",current_reference_price=89.0100"
                ",cross_type=O"
                ",price_variation_indicator=A");
}

/**
 * @test Verify that imbalance_direction_t works as expected.
 */
BOOST_AUTO_TEST_CASE(simple_imbalance_direction) {
  using namespace jb::itch5;
  BOOST_CHECK_NO_THROW(imbalance_direction_t(u'B'));
  BOOST_CHECK_NO_THROW(imbalance_direction_t(u'S'));
  BOOST_CHECK_NO_THROW(imbalance_direction_t(u'N'));
  BOOST_CHECK_NO_THROW(imbalance_direction_t(u'O'));
  BOOST_CHECK_THROW(imbalance_direction_t(u'*'), std::runtime_error);
}

/**
 * @test Verify that price_variation_indicator_t works as expected.
 */
BOOST_AUTO_TEST_CASE(simple_price_variation_indicator) {
  using namespace jb::itch5;
  BOOST_CHECK_NO_THROW(price_variation_indicator_t(u'L'));
  BOOST_CHECK_NO_THROW(price_variation_indicator_t(u'1'));
  BOOST_CHECK_NO_THROW(price_variation_indicator_t(u'2'));
  BOOST_CHECK_NO_THROW(price_variation_indicator_t(u'3'));
  BOOST_CHECK_NO_THROW(price_variation_indicator_t(u'4'));
  BOOST_CHECK_NO_THROW(price_variation_indicator_t(u'5'));
  BOOST_CHECK_NO_THROW(price_variation_indicator_t(u'6'));
  BOOST_CHECK_NO_THROW(price_variation_indicator_t(u'7'));
  BOOST_CHECK_NO_THROW(price_variation_indicator_t(u'8'));
  BOOST_CHECK_NO_THROW(price_variation_indicator_t(u'9'));
  BOOST_CHECK_NO_THROW(price_variation_indicator_t(u'A'));
  BOOST_CHECK_NO_THROW(price_variation_indicator_t(u'B'));
  BOOST_CHECK_NO_THROW(price_variation_indicator_t(u'C'));
  BOOST_CHECK_NO_THROW(price_variation_indicator_t(u' '));
  BOOST_CHECK_THROW(price_variation_indicator_t(u'*'), std::runtime_error);
}
