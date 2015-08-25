#include <jb/itch5/stock_trading_action_message.hpp>
#include <jb/itch5/testing_data.hpp>

#include <boost/test/unit_test.hpp>

namespace {
// A sample message for testing
char const buf[] =
    u8"H"                 // Message Type
    JB_ITCH5_TEST_HEADER  // Common test header
    "HSART   "            // Stock
    "T"                   // Trading State
    "\x00"                // Reserved
    "MWC1"                // Reason
    ;
std::size_t const bufsize = sizeof(buf) - 1;
} // anonymous namespace

/**
 * @test Verify that the jb::itch5::stock_trading_action_message decoder works
 * as expected.
 */
BOOST_AUTO_TEST_CASE(decode_stock_trading_action_message) {
  using namespace jb::itch5;
  using namespace std::chrono;

  auto expected_ts = duration_cast<nanoseconds>(
      hours(11) + minutes(32) + seconds(31) + nanoseconds(123456789L));

  auto x = decoder<true,stock_trading_action_message>::r(bufsize, buf, 0);
  BOOST_CHECK_EQUAL(
      x.header.message_type, stock_trading_action_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.stock, "HSART");
  BOOST_CHECK_EQUAL(x.trading_state, u'T');
  BOOST_CHECK_EQUAL(x.reason, u8"MWC1");

  x = decoder<false,stock_trading_action_message>::r(bufsize, buf, 0);
  BOOST_CHECK_EQUAL(
      x.header.message_type, stock_trading_action_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.stock, "HSART");
  BOOST_CHECK_EQUAL(x.trading_state, u'T');
  BOOST_CHECK_EQUAL(x.reason, u8"MWC1");
}

/**
 * @test Verify that jb::itch5::stock_trading_action_message iostream
 * operator works as expected.
 */
BOOST_AUTO_TEST_CASE(stream_stock_trading_action_message) {
  using namespace std::chrono;
  using namespace jb::itch5;

  auto tmp = decoder<false,stock_trading_action_message>::r(bufsize, buf, 0);
  std::ostringstream os;
  os << tmp;
  BOOST_CHECK_EQUAL(
      os.str(), "message_type=H,stock_locate=0"
      ",tracking_number=1,timestamp=113231.123456789"
      ",stock=HSART"
      ",trading_state=T"
      ",reserved=0"
      ",reason=MWC1"
      );
}

/**
 * @test Verify that trading_state_t works as expected.
 */
BOOST_AUTO_TEST_CASE(simple_trading_state) {
  using namespace jb::itch5;
  BOOST_CHECK_NO_THROW(trading_state_t(u'H'));
  BOOST_CHECK_NO_THROW(trading_state_t(u'P'));
  BOOST_CHECK_NO_THROW(trading_state_t(u'Q'));
  BOOST_CHECK_NO_THROW(trading_state_t(u'T'));
  BOOST_CHECK_THROW(trading_state_t(u' '), std::runtime_error);
}
