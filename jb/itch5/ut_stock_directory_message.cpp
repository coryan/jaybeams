#include <jb/itch5/stock_directory_message.hpp>
#include <jb/itch5/testing_data.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that the jb::itch5::stock_directory_message decoder works
 * as expected.
 */
BOOST_AUTO_TEST_CASE(decode_stock_directory_message) {
  using namespace jb::itch5;
  using namespace std::chrono;

  auto buf = jb::itch5::testing::stock_directory();
  auto expected_ts = jb::itch5::testing::expected_ts();

  auto x = decoder<true, stock_directory_message>::r(buf.second, buf.first, 0);
  BOOST_CHECK_EQUAL(
      x.header.message_type, stock_directory_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.stock, "HSART");
  BOOST_CHECK_EQUAL(x.market_category, u'G');
  BOOST_CHECK_EQUAL(x.financial_status_indicator, u'N');
  BOOST_CHECK_EQUAL(x.round_lot_size, 100);
  BOOST_CHECK_EQUAL(x.roundlots_only, u'N');
  BOOST_CHECK_EQUAL(x.issue_classification, u'O');
  BOOST_CHECK_EQUAL(x.issue_subtype, u8"C");
  BOOST_CHECK_EQUAL(x.authenticity, u'P');
  BOOST_CHECK_EQUAL(x.short_sale_threshold_indicator, u'N');
  BOOST_CHECK_EQUAL(x.ipo_flag, u'N');
  BOOST_CHECK_EQUAL(x.luld_reference_price_tier, u'1');
  BOOST_CHECK_EQUAL(x.etp_flag, u'N');
  BOOST_CHECK_EQUAL(x.etp_leverage_factor, 0);
  BOOST_CHECK_EQUAL(x.inverse_indicator, u'N');

  x = decoder<false, stock_directory_message>::r(buf.second, buf.first, 0);
  BOOST_CHECK_EQUAL(
      x.header.message_type, stock_directory_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.stock, "HSART");
  BOOST_CHECK_EQUAL(x.market_category, u'G');
  BOOST_CHECK_EQUAL(x.financial_status_indicator, u'N');
  BOOST_CHECK_EQUAL(x.round_lot_size, 100);
  BOOST_CHECK_EQUAL(x.roundlots_only, u'N');
  BOOST_CHECK_EQUAL(x.issue_classification, u'O');
  BOOST_CHECK_EQUAL(x.issue_subtype, u8"C");
  BOOST_CHECK_EQUAL(x.authenticity, u'P');
  BOOST_CHECK_EQUAL(x.short_sale_threshold_indicator, u'N');
  BOOST_CHECK_EQUAL(x.ipo_flag, u'N');
  BOOST_CHECK_EQUAL(x.luld_reference_price_tier, u'1');
  BOOST_CHECK_EQUAL(x.etp_flag, u'N');
  BOOST_CHECK_EQUAL(x.etp_leverage_factor, 0);
  BOOST_CHECK_EQUAL(x.inverse_indicator, u'N');
}

/**
 * @test Verify that jb::itch5::stock_directory_message iostream operator works
 * as expected.
 */
BOOST_AUTO_TEST_CASE(stream_stock_directory_message) {
  using namespace std::chrono;
  using namespace jb::itch5;

  auto buf = jb::itch5::testing::stock_directory();
  auto tmp =
      decoder<false, stock_directory_message>::r(buf.second, buf.first, 0);
  std::ostringstream os;
  os << tmp;
  BOOST_CHECK_EQUAL(
      os.str(), "message_type=R,stock_locate=0"
                ",tracking_number=1,timestamp=113231.123456789"
                ",stock=HSART"
                ",market_category=G"
                ",financial_status_indicator=N"
                ",round_lot_size=100"
                ",roundlots_only=N"
                ",issue_classification=O"
                ",issue_subtype=C"
                ",authenticity=P"
                ",short_sale_threshold_indicator=N"
                ",ipo_flag=N"
                ",luld_reference_price_tier=1"
                ",etp_flag=N"
                ",etp_leverage_factor=0"
                ",inverse_indicator=N");
}

/**
 * @test Verify that market_category_t works as expected.
 */
BOOST_AUTO_TEST_CASE(simple_market_category) {
  using namespace jb::itch5;
  BOOST_CHECK_NO_THROW(market_category_t(u'Q'));
  BOOST_CHECK_NO_THROW(market_category_t(u'G'));
  BOOST_CHECK_NO_THROW(market_category_t(u'S'));
  BOOST_CHECK_NO_THROW(market_category_t(u'N'));
  BOOST_CHECK_NO_THROW(market_category_t(u'A'));
  BOOST_CHECK_NO_THROW(market_category_t(u'P'));
  BOOST_CHECK_NO_THROW(market_category_t(u'Z'));
  BOOST_CHECK_NO_THROW(market_category_t(u' '));
  BOOST_CHECK_THROW(market_category_t(u'X'), std::runtime_error);
}

/**
 * @test Verify that financial_status_indicator_t works as expected.
 */
BOOST_AUTO_TEST_CASE(simple_financial_status_indicator) {
  using namespace jb::itch5;
  BOOST_CHECK_NO_THROW(financial_status_indicator_t(u'Q'));
  BOOST_CHECK_NO_THROW(financial_status_indicator_t(u'S'));
  BOOST_CHECK_NO_THROW(financial_status_indicator_t(u' '));
  BOOST_CHECK_THROW(financial_status_indicator_t(u'X'), std::runtime_error);
}

/**
 * @test Verify that roundlots_only_t works as expected.
 */
BOOST_AUTO_TEST_CASE(simple_roundlots_only) {
  using namespace jb::itch5;
  BOOST_CHECK_NO_THROW(roundlots_only_t(u'Y'));
  BOOST_CHECK_NO_THROW(roundlots_only_t(u'N'));
  BOOST_CHECK_THROW(roundlots_only_t(u'X'), std::runtime_error);
}

/**
 * @test Verify that issue_classification_t works as expected.
 */
BOOST_AUTO_TEST_CASE(simple_issue_classification) {
  using namespace jb::itch5;
  BOOST_CHECK_NO_THROW(issue_classification_t(u'A'));
  BOOST_CHECK_NO_THROW(issue_classification_t(u'B'));
  BOOST_CHECK_NO_THROW(issue_classification_t(u'W'));
  BOOST_CHECK_THROW(issue_classification_t(u' '), std::runtime_error);
}

/**
 * @test Verify that short_sale_threshold_indicator_t works as expected.
 */
BOOST_AUTO_TEST_CASE(simple_short_sale_threshold_indicator) {
  using namespace jb::itch5;
  BOOST_CHECK_NO_THROW(short_sale_threshold_indicator_t(u'Y'));
  BOOST_CHECK_NO_THROW(short_sale_threshold_indicator_t(u'N'));
  BOOST_CHECK_NO_THROW(short_sale_threshold_indicator_t(u' '));
  BOOST_CHECK_THROW(short_sale_threshold_indicator_t(u'X'), std::runtime_error);
}

/**
 * @test Verify that luld_reference_price_tier_t works as expected.
 */
BOOST_AUTO_TEST_CASE(simple_luld_reference_price_tier) {
  using namespace jb::itch5;
  BOOST_CHECK_NO_THROW(luld_reference_price_tier_t(u'1'));
  BOOST_CHECK_NO_THROW(luld_reference_price_tier_t(u'2'));
  BOOST_CHECK_NO_THROW(luld_reference_price_tier_t(u' '));
  BOOST_CHECK_THROW(luld_reference_price_tier_t(u'*'), std::runtime_error);
}

/**
 * @test Verify that etp_flag_t works as expected.
 */
BOOST_AUTO_TEST_CASE(simple_etp_flag) {
  using namespace jb::itch5;
  BOOST_CHECK_NO_THROW(etp_flag_t(u'Y'));
  BOOST_CHECK_NO_THROW(etp_flag_t(u'N'));
  BOOST_CHECK_NO_THROW(etp_flag_t(u' '));
  BOOST_CHECK_THROW(etp_flag_t(u'*'), std::runtime_error);
}

/**
 * @test Verify that inverse_indicator_t works as expected.
 */
BOOST_AUTO_TEST_CASE(simple_inverse_indicator) {
  using namespace jb::itch5;
  BOOST_CHECK_NO_THROW(inverse_indicator_t(u'Y'));
  BOOST_CHECK_NO_THROW(inverse_indicator_t(u'N'));
  BOOST_CHECK_THROW(inverse_indicator_t(u'*'), std::runtime_error);
}
