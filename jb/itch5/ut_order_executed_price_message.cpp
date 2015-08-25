#include <jb/itch5/order_executed_price_message.hpp>
#include <jb/itch5/testing_data.hpp>

#include <boost/test/unit_test.hpp>

namespace {
// A sample message for testing
char const buf[] =
    u8"C"                 // Message Type
    JB_ITCH5_TEST_HEADER  // Common test header
    "\x00\x00\x00\x00"
    "\x00\x00\x00\x2A"    // Order Reference Number (42)
    "\x00\x00\x01\x2C"    // Executed Shares (300)
    "\x00\x00\x00\x00"
    "\x00\x00\x01\x3D"    // Match Number (317)
    "Y"                   // Printable (Y)
    "\x00\x12\xC6\xA4"    // Execution Price (123.0500)
    ;
std::size_t const bufsize = sizeof(buf) - 1;
} // anonymous namespace

/**
 * @test Verify that the jb::itch5::order_executed_price_message decoder works
 * as expected.
 */
BOOST_AUTO_TEST_CASE(decode_order_executed_price_message) {
  using namespace jb::itch5;
  using namespace std::chrono;

  auto expected_ts = duration_cast<nanoseconds>(
      hours(11) + minutes(32) + seconds(31) + nanoseconds(123456789L));

  auto x = decoder<true,order_executed_price_message>::r(bufsize, buf, 0);
  BOOST_CHECK_EQUAL(
      x.header.message_type, order_executed_price_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.order_reference_number, 42ULL);
  BOOST_CHECK_EQUAL(x.executed_shares, 300);
  BOOST_CHECK_EQUAL(x.match_number, 317ULL);
  BOOST_CHECK_EQUAL(x.printable, printable_t(u'Y'));
  BOOST_CHECK_EQUAL(x.execution_price, price4_t(1230500));

  x = decoder<false,order_executed_price_message>::r(bufsize, buf, 0);
  BOOST_CHECK_EQUAL(
      x.header.message_type, order_executed_price_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.order_reference_number, 42ULL);
  BOOST_CHECK_EQUAL(x.order_reference_number, 42ULL);
  BOOST_CHECK_EQUAL(x.executed_shares, 300);
  BOOST_CHECK_EQUAL(x.match_number, 317ULL);
  BOOST_CHECK_EQUAL(x.printable, printable_t(u'Y'));
  BOOST_CHECK_EQUAL(x.execution_price, price4_t(1230500));
}

/**
 * @test Verify that jb::itch5::order_executed_price_message iostream
 * operator works as expected.
 */
BOOST_AUTO_TEST_CASE(stream_order_executed_price_message) {
  using namespace std::chrono;
  using namespace jb::itch5;

  auto tmp = decoder<false,order_executed_price_message>::r(bufsize, buf, 0);
  std::ostringstream os;
  os << tmp;
  BOOST_CHECK_EQUAL(
      os.str(), "message_type=C,stock_locate=0"
      ",tracking_number=1,timestamp=113231.123456789"
      ",order_reference_number=42"
      ",executed_shares=300"
      ",match_number=317"
      ",printable=Y"
      ",execution_price=123.0500"
      );
}
