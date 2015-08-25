#include <jb/itch5/order_replace_message.hpp>
#include <jb/itch5/testing_data.hpp>

#include <boost/test/unit_test.hpp>

namespace {
// A sample message for testing
char const buf[] =
    u8"U"                 // Message Type
    JB_ITCH5_TEST_HEADER  // Common test header
    "\x00\x00\x00\x00"
    "\x00\x00\x00\x2A"    // Original Order Reference Number (42)
    "\x00\x00\x00\x00"
    "\x00\x00\x10\x92"    // New Order Reference Number (4242)
    "\x00\x00\x00\x64"    // Shares (100)
    "\x00\x23\xB6\xF8"    // Price (234.0600)
    ;
std::size_t const bufsize = sizeof(buf) - 1;
} // anonymous namespace

/**
 * @test Verify that the jb::itch5::order_replace_message decoder works
 * as expected.
 */
BOOST_AUTO_TEST_CASE(decode_order_replace_message) {
  using namespace jb::itch5;
  using namespace std::chrono;

  auto expected_ts = duration_cast<nanoseconds>(
      hours(11) + minutes(32) + seconds(31) + nanoseconds(123456789L));

  auto x = decoder<true,order_replace_message>::r(bufsize, buf, 0);
  BOOST_CHECK_EQUAL(
      x.header.message_type, order_replace_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.original_order_reference_number, 42ULL);
  BOOST_CHECK_EQUAL(x.new_order_reference_number, 4242ULL);
  BOOST_CHECK_EQUAL(x.shares, 100);
  BOOST_CHECK_EQUAL(x.price, price4_t(2340600));

  x = decoder<false,order_replace_message>::r(bufsize, buf, 0);
  BOOST_CHECK_EQUAL(
      x.header.message_type, order_replace_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.original_order_reference_number, 42ULL);
  BOOST_CHECK_EQUAL(x.new_order_reference_number, 4242ULL);
  BOOST_CHECK_EQUAL(x.shares, 100);
  BOOST_CHECK_EQUAL(x.price, price4_t(2340600));
}

/**
 * @test Verify that jb::itch5::order_replace_message iostream
 * operator works as expected.
 */
BOOST_AUTO_TEST_CASE(stream_order_replace_message) {
  using namespace std::chrono;
  using namespace jb::itch5;

  auto tmp = decoder<false,order_replace_message>::r(bufsize, buf, 0);
  std::ostringstream os;
  os << tmp;
  BOOST_CHECK_EQUAL(
      os.str(), "message_type=U,stock_locate=0"
      ",tracking_number=1,timestamp=113231.123456789"
      ",original_order_reference_number=42"
      ",new_order_reference_number=4242"
      ",shares=100"
      ",price=234.0600"
      );
}
