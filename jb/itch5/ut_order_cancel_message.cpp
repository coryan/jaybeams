#include <jb/itch5/order_cancel_message.hpp>
#include <jb/itch5/testing_data.hpp>

#include <boost/test/unit_test.hpp>

namespace {
// A sample message for testing
char const buf[] =
    u8"X"                 // Message Type
    JB_ITCH5_TEST_HEADER  // Common test header
    "\x00\x00\x00\x00"
    "\x00\x00\x00\x2A"    // Order Reference Number (42)
    "\x00\x00\x01\x2C"    // Canceled Shares (300)
    ;
std::size_t const bufsize = sizeof(buf) - 1;
} // anonymous namespace

/**
 * @test Verify that the jb::itch5::order_cancel_message decoder works
 * as expected.
 */
BOOST_AUTO_TEST_CASE(decode_order_cancel_message) {
  using namespace jb::itch5;
  using namespace std::chrono;

  auto expected_ts = duration_cast<nanoseconds>(
      hours(11) + minutes(32) + seconds(31) + nanoseconds(123456789L));

  auto x = decoder<true,order_cancel_message>::r(bufsize, buf, 0);
  BOOST_CHECK_EQUAL(
      x.header.message_type, order_cancel_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.order_reference_number, 42ULL);
  BOOST_CHECK_EQUAL(x.canceled_shares, 300);

  x = decoder<false,order_cancel_message>::r(bufsize, buf, 0);
  BOOST_CHECK_EQUAL(
      x.header.message_type, order_cancel_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.order_reference_number, 42ULL);
  BOOST_CHECK_EQUAL(x.order_reference_number, 42ULL);
  BOOST_CHECK_EQUAL(x.canceled_shares, 300);
}

/**
 * @test Verify that jb::itch5::order_cancel_message iostream
 * operator works as expected.
 */
BOOST_AUTO_TEST_CASE(stream_order_cancel_message) {
  using namespace std::chrono;
  using namespace jb::itch5;

  auto tmp = decoder<false,order_cancel_message>::r(bufsize, buf, 0);
  std::ostringstream os;
  os << tmp;
  BOOST_CHECK_EQUAL(
      os.str(), "message_type=X,stock_locate=0"
      ",tracking_number=1,timestamp=113231.123456789"
      ",order_reference_number=42"
      ",canceled_shares=300"
      );
}
