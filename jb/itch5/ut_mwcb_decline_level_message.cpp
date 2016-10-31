#include <jb/itch5/mwcb_decline_level_message.hpp>
#include <jb/itch5/testing_data.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that the jb::itch5::mwcb_decline_level_message decoder works
 * as expected.
 */
BOOST_AUTO_TEST_CASE(decode_mwcb_decline_level_message) {
  using namespace jb::itch5;
  using namespace std::chrono;

  auto buf = jb::itch5::testing::mwcb_decline_level();
  auto expected_ts = jb::itch5::testing::expected_ts();

  auto x =
      decoder<true, mwcb_decline_level_message>::r(buf.second, buf.first, 0);
  BOOST_CHECK_EQUAL(
      x.header.message_type, mwcb_decline_level_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.level_1, price8_t(500001000000LL));
  BOOST_CHECK_EQUAL(x.level_2, price8_t(400001020000LL));
  BOOST_CHECK_EQUAL(x.level_3, price8_t(300001020300LL));

  x = decoder<false, mwcb_decline_level_message>::r(buf.second, buf.first, 0);
  BOOST_CHECK_EQUAL(
      x.header.message_type, mwcb_decline_level_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.level_1, price8_t(500001000000LL));
  BOOST_CHECK_EQUAL(x.level_2, price8_t(400001020000LL));
  BOOST_CHECK_EQUAL(x.level_3, price8_t(300001020300LL));
}

/**
 * @test Verify that jb::itch5::mwcb_decline_level_message iostream
 * operator works as expected.
 */
BOOST_AUTO_TEST_CASE(stream_mwcb_decline_level_message) {
  using namespace std::chrono;
  using namespace jb::itch5;

  auto buf = jb::itch5::testing::mwcb_decline_level();
  auto tmp =
      decoder<false, mwcb_decline_level_message>::r(buf.second, buf.first, 0);
  std::ostringstream os;
  os << tmp;
  BOOST_CHECK_EQUAL(
      os.str(), "message_type=V,stock_locate=0"
                ",tracking_number=1,timestamp=113231.123456789"
                ",level_1=5000.01000000"
                ",level_2=4000.01020000"
                ",level_3=3000.01020300");
}
