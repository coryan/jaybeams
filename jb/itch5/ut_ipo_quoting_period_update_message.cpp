#include <jb/itch5/ipo_quoting_period_update_message.hpp>
#include <jb/itch5/testing_data.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that the jb::itch5::ipo_quoting_period_update_message
 * decoder works as expected.
 */
BOOST_AUTO_TEST_CASE(decode_ipo_quoting_period_update_message) {
  using namespace jb::itch5;
  using namespace std::chrono;

  auto buf = jb::itch5::testing::ipo_quoting_period_update();
  auto expected_ts = jb::itch5::testing::expected_ts();
  auto expected_release =
      duration_cast<seconds>(hours(13) + minutes(43) + seconds(25));

  auto x = decoder<true, ipo_quoting_period_update_message>::r(
      buf.second, buf.first, 0);
  BOOST_CHECK_EQUAL(
      x.header.message_type, ipo_quoting_period_update_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.stock, "HSART");
  BOOST_CHECK_EQUAL(
      x.ipo_quotation_release_time.seconds().count(), expected_release.count());
  BOOST_CHECK_EQUAL(x.ipo_quotation_release_qualifier, u'A');
  BOOST_CHECK_EQUAL(x.ipo_price, price4_t(1230500));

  x = decoder<false, ipo_quoting_period_update_message>::r(
      buf.second, buf.first, 0);
  BOOST_CHECK_EQUAL(
      x.header.message_type, ipo_quoting_period_update_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.stock, "HSART");
  BOOST_CHECK_EQUAL(
      x.ipo_quotation_release_time.seconds().count(), expected_release.count());
  BOOST_CHECK_EQUAL(x.ipo_quotation_release_qualifier, u'A');
  BOOST_CHECK_EQUAL(x.ipo_price, price4_t(1230500));
}

/**
 * @test Verify that jb::itch5::ipo_quoting_period_update_message iostream
 * operator works as expected.
 */
BOOST_AUTO_TEST_CASE(stream_ipo_quoting_period_update_message) {
  using namespace std::chrono;
  using namespace jb::itch5;

  auto buf = jb::itch5::testing::ipo_quoting_period_update();
  auto tmp = decoder<false, ipo_quoting_period_update_message>::r(
      buf.second, buf.first, 0);
  std::ostringstream os;
  os << tmp;
  BOOST_CHECK_EQUAL(
      os.str(), "message_type=K,stock_locate=0"
                ",tracking_number=1,timestamp=113231.123456789"
                ",stock=HSART"
                ",ipo_quotation_release_time=13:43:25"
                ",ipo_quotation_release_qualifier=A"
                ",ipo_price=123.0500");
}

/**
 * @test Verify that ipo_quotation_release_qualifier_t works as expected.
 */
BOOST_AUTO_TEST_CASE(simple_ipo_quotation_release_qualifier) {
  using namespace jb::itch5;
  BOOST_CHECK_NO_THROW(ipo_quotation_release_qualifier_t(u'A'));
  BOOST_CHECK_NO_THROW(ipo_quotation_release_qualifier_t(u'C'));
  BOOST_CHECK_THROW(
      ipo_quotation_release_qualifier_t(u'*'), std::runtime_error);
}
