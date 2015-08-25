#include <jb/itch5/mwcb_decline_level_message.hpp>
#include <jb/itch5/testing_data.hpp>

#include <boost/test/unit_test.hpp>

namespace {
// A sample message for testing
char const buf[] =
    u8"V"                 // Message Type
    JB_ITCH5_TEST_HEADER  // Common test header
    "\x00\x00\x00\x74\x6A\x61\xCA\x40" // Level 1 (5000.01)
    "\x00\x00\x00\x5D\x21\xEB\x30\x60" // Level 2 (4000.0102)
    "\x00\x00\x00\x45\xD9\x74\x49\x8C" // Level 3 (3000.010203)
    ;
std::size_t const bufsize = sizeof(buf) - 1;
} // anonymous namespace

/**
 * @test Verify that the jb::itch5::mwcb_decline_level_message decoder works
 * as expected.
 */
BOOST_AUTO_TEST_CASE(decode_mwcb_decline_level_message) {
  using namespace jb::itch5;
  using namespace std::chrono;

  auto expected_ts = duration_cast<nanoseconds>(
      hours(11) + minutes(32) + seconds(31) + nanoseconds(123456789L));

  auto x = decoder<true,mwcb_decline_level_message>::r(bufsize, buf, 0);
  BOOST_CHECK_EQUAL(
      x.header.message_type, mwcb_decline_level_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.level_1, price8_t(500001000000LL));
  BOOST_CHECK_EQUAL(x.level_2, price8_t(400001020000LL));
  BOOST_CHECK_EQUAL(x.level_3, price8_t(300001020300LL));

  x = decoder<false,mwcb_decline_level_message>::r(bufsize, buf, 0);
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

  auto tmp = decoder<false,mwcb_decline_level_message>::r(bufsize, buf, 0);
  std::ostringstream os;
  os << tmp;
  BOOST_CHECK_EQUAL(
      os.str(), "message_type=V,stock_locate=0"
      ",tracking_number=1,timestamp=113231.123456789"
      ",level_1=5000.01000000"
      ",level_2=4000.01020000"
      ",level_3=3000.01020300"
      );
}
