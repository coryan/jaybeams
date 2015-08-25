#include <jb/itch5/ipo_quoting_period_update_message.hpp>
#include <jb/itch5/testing_data.hpp>

#include <boost/test/unit_test.hpp>

namespace {
// A sample message for testing
char const buf[] =
    u8"K"                 // Message Type
    JB_ITCH5_TEST_HEADER  // Common test header
    "HSART   "            // Stock
    "\x00\x00\xC0\xFD"    // IPO Quotation Release Time (13:43:25)
    "A"                   // IPO Quotation Release Qualifier
    "\x00\x12\xC6\xA4"    // IPO Price (123.0500)
    ;
std::size_t const bufsize = sizeof(buf) - 1;
} // anonymous namespace

/**
 * @test Verify that the jb::itch5::ipo_quoting_period_update_message decoder works
 * as expected.
 */
BOOST_AUTO_TEST_CASE(decode_ipo_quoting_period_update_message) {
  using namespace jb::itch5;
  using namespace std::chrono;

  auto expected_ts = duration_cast<nanoseconds>(
      hours(11) + minutes(32) + seconds(31) + nanoseconds(123456789L));

  auto expected_release = duration_cast<seconds>(
      hours(13) + minutes(43) + seconds(25));

  auto x = decoder<true,ipo_quoting_period_update_message>::r(
      bufsize, buf, 0);
  BOOST_CHECK_EQUAL(
      x.header.message_type, ipo_quoting_period_update_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.stock, "HSART");
  BOOST_CHECK_EQUAL(
      x.ipo_quotation_release_time.seconds().count(), expected_release.count());
  BOOST_CHECK_EQUAL(x.ipo_quotation_release_qualifier, u'A');
  BOOST_CHECK_EQUAL(x.ipo_price, price4_t(1230500));

  x = decoder<false,ipo_quoting_period_update_message>::r(bufsize, buf, 0);
  BOOST_CHECK_EQUAL(
      x.header.message_type, ipo_quoting_period_update_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.stock, "HSART");
  BOOST_CHECK_EQUAL(
      x.ipo_quotation_release_time.seconds().count(), expected_release.count());
  BOOST_CHECK_EQUAL(x.ipo_quotation_release_qualifier, u'A');
  BOOST_CHECK_EQUAL(x.ipo_price, price4_t(1230500));
}

/**
 * @test Verify that jb::itch5::ipo_quoting_period_update_message iostream
 * operator works as expected.
 */
BOOST_AUTO_TEST_CASE(stream_ipo_quoting_period_update_message) {
  using namespace std::chrono;
  using namespace jb::itch5;

  auto tmp = decoder<false,ipo_quoting_period_update_message>::r(
      bufsize, buf, 0);
  std::ostringstream os;
  os << tmp;
  BOOST_CHECK_EQUAL(
      os.str(), "message_type=K,stock_locate=0"
      ",tracking_number=1,timestamp=113231.123456789"
      ",stock=HSART"
      ",ipo_quotation_release_time=13:43:25"
      ",ipo_quotation_release_qualifier=A"
      ",ipo_price=123.0500"
      );
}

/**
 * @test Verify that ipo_quotation_release_qualifier_t works as expected.
 */
BOOST_AUTO_TEST_CASE(simple_ipo_quotation_release_qualifier) {
  using namespace jb::itch5;
  BOOST_CHECK_NO_THROW(ipo_quotation_release_qualifier_t(u'A'));
  BOOST_CHECK_NO_THROW(ipo_quotation_release_qualifier_t(u'C'));
  BOOST_CHECK_THROW(
      ipo_quotation_release_qualifier_t(u'*'), std::runtime_error);
}
