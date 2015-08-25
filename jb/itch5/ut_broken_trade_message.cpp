#include <jb/itch5/broken_trade_message.hpp>
#include <jb/itch5/testing_data.hpp>

#include <boost/test/unit_test.hpp>

namespace {
// A sample message for testing
char const buf[] =
    u8"B"                 // Message Type
    JB_ITCH5_TEST_HEADER  // Common test header
    "\x00\x00\x00\x00"
    "\x00\x23\xB6\xF8"    // Match Number (2340600)
    ;
std::size_t const bufsize = sizeof(buf) - 1;
} // anonymous namespace

/**
 * @test Verify that the jb::itch5::broken_trade_message decoder works
 * as expected.
 */
BOOST_AUTO_TEST_CASE(decode_broken_trade_message) {
  using namespace jb::itch5;
  using namespace std::chrono;

  auto expected_ts = duration_cast<nanoseconds>(
      hours(11) + minutes(32) + seconds(31) + nanoseconds(123456789L));

  auto x = decoder<true,broken_trade_message>::r(bufsize, buf, 0);
  BOOST_CHECK_EQUAL(
      x.header.message_type, broken_trade_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.match_number, 2340600ULL);

  x = decoder<false,broken_trade_message>::r(bufsize, buf, 0);
  BOOST_CHECK_EQUAL(
      x.header.message_type, broken_trade_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.match_number, 2340600ULL);
}

/**
 * @test Verify that jb::itch5::broken_trade_message iostream
 * operator works as expected.
 */
BOOST_AUTO_TEST_CASE(stream_broken_trade_message) {
  using namespace std::chrono;
  using namespace jb::itch5;

  auto tmp = decoder<false,broken_trade_message>::r(bufsize, buf, 0);
  std::ostringstream os;
  os << tmp;
  BOOST_CHECK_EQUAL(
      os.str(), "message_type=B,stock_locate=0"
      ",tracking_number=1,timestamp=113231.123456789"
      ",match_number=2340600"
      );
}
