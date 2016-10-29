#include <jb/itch5/market_participant_position_message.hpp>
#include <jb/itch5/testing_data.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that the jb::itch5::market_participant_position_message
 * decoder works as expected.
 */
BOOST_AUTO_TEST_CASE(decode_market_participant_position_message) {
  using namespace jb::itch5;
  using namespace std::chrono;

  auto buf = jb::itch5::testing::market_participant_position();
  auto expected_ts = jb::itch5::testing::expected_ts();

  auto x = decoder<true, market_participant_position_message>::r(buf.second,
                                                                 buf.first, 0);
  BOOST_CHECK_EQUAL(x.header.message_type,
                    market_participant_position_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.mpid, "LOOF");
  BOOST_CHECK_EQUAL(x.stock, "HSART");
  BOOST_CHECK_EQUAL(x.primary_market_maker, u'N');
  BOOST_CHECK_EQUAL(x.market_maker_mode, u'N');
  BOOST_CHECK_EQUAL(x.market_participant_state, u'A');

  x = decoder<false, market_participant_position_message>::r(buf.second,
                                                             buf.first, 0);
  BOOST_CHECK_EQUAL(x.header.message_type,
                    market_participant_position_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.mpid, "LOOF");
  BOOST_CHECK_EQUAL(x.stock, "HSART");
  BOOST_CHECK_EQUAL(x.primary_market_maker, u'N');
  BOOST_CHECK_EQUAL(x.market_maker_mode, u'N');
  BOOST_CHECK_EQUAL(x.market_participant_state, u'A');
}

/**
 * @test Verify that jb::itch5::market_participant_position_message iostream
 * operator works as expected.
 */
BOOST_AUTO_TEST_CASE(stream_market_participant_position_message) {
  using namespace std::chrono;
  using namespace jb::itch5;

  auto buf = jb::itch5::testing::market_participant_position();
  auto tmp = decoder<false, market_participant_position_message>::r(
      buf.second, buf.first, 0);
  std::ostringstream os;
  os << tmp;
  BOOST_CHECK_EQUAL(os.str(), "message_type=L,stock_locate=0"
                              ",tracking_number=1,timestamp=113231.123456789"
                              ",mpid=LOOF"
                              ",stock=HSART"
                              ",primary_market_maker=N"
                              ",market_maker_mode=N"
                              ",market_participant_state=A");
}

/**
 * @test Verify that primary_market_maker_t works as expected.
 */
BOOST_AUTO_TEST_CASE(simple_primary_market_maker) {
  using namespace jb::itch5;
  BOOST_CHECK_NO_THROW(primary_market_maker_t(u'Y'));
  BOOST_CHECK_NO_THROW(primary_market_maker_t(u'N'));
  BOOST_CHECK_THROW(primary_market_maker_t(u'*'), std::runtime_error);
}

/**
 * @test Verify that market_maker_mode_t works as expected.
 */
BOOST_AUTO_TEST_CASE(simple_market_maker_mode) {
  using namespace jb::itch5;
  BOOST_CHECK_NO_THROW(market_maker_mode_t(u'N'));
  BOOST_CHECK_NO_THROW(market_maker_mode_t(u'P'));
  BOOST_CHECK_THROW(market_maker_mode_t(u'*'), std::runtime_error);
}

/**
 * @test Verify that market_participant_state_t works as expected.
 */
BOOST_AUTO_TEST_CASE(simple_market_participant_state) {
  using namespace jb::itch5;
  BOOST_CHECK_NO_THROW(market_participant_state_t(u'A'));
  BOOST_CHECK_NO_THROW(market_participant_state_t(u'E'));
  BOOST_CHECK_THROW(market_participant_state_t(u'*'), std::runtime_error);
}
