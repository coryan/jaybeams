#include <jb/itch5/mwcb_breach_message.hpp>
#include <jb/itch5/testing_data.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that the jb::itch5::mwcb_breach_message decoder works
 * as expected.
 */
BOOST_AUTO_TEST_CASE(decode_mwcb_breach_message) {
  using namespace jb::itch5;
  using namespace std::chrono;

  auto buf = jb::itch5::testing::mwcb_breach();
  auto expected_ts = jb::itch5::testing::expected_ts();

  auto x = decoder<true, mwcb_breach_message>::r(buf.second, buf.first, 0);
  BOOST_CHECK_EQUAL(x.header.message_type, mwcb_breach_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.breached_level, u'2');

  x = decoder<false, mwcb_breach_message>::r(buf.second, buf.first, 0);
  BOOST_CHECK_EQUAL(x.header.message_type, mwcb_breach_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.breached_level, u'2');
}

/**
 * @test Verify that jb::itch5::mwcb_breach_message iostream
 * operator works as expected.
 */
BOOST_AUTO_TEST_CASE(stream_mwcb_breach_message) {
  using namespace std::chrono;
  using namespace jb::itch5;

  auto buf = jb::itch5::testing::mwcb_breach();
  auto tmp = decoder<false, mwcb_breach_message>::r(buf.second, buf.first, 0);
  std::ostringstream os;
  os << tmp;
  BOOST_CHECK_EQUAL(
      os.str(), "message_type=W,stock_locate=0"
                ",tracking_number=1,timestamp=113231.123456789"
                ",breached_level=2");
}

/**
 * @test Verify that breached_level_t works as expected.
 */
BOOST_AUTO_TEST_CASE(simple_breached_level) {
  using namespace jb::itch5;
  BOOST_CHECK_NO_THROW(breached_level_t(u'1'));
  BOOST_CHECK_NO_THROW(breached_level_t(u'2'));
  BOOST_CHECK_NO_THROW(breached_level_t(u'3'));
  BOOST_CHECK_THROW(breached_level_t(u'*'), std::runtime_error);
}
