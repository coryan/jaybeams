#include <jb/itch5/reg_sho_restriction_message.hpp>
#include <jb/itch5/testing_data.hpp>

#include <boost/test/unit_test.hpp>

namespace {
// A sample message for testing
char const buf[] =
    u8"Y"                 // Message Type
    JB_ITCH5_TEST_HEADER  // Common test header
    "HSART   "            // Stock
    "0"                   // Reg SHO Action
    ;
std::size_t const bufsize = sizeof(buf) - 1;
} // anonymous namespace

/**
 * @test Verify that the jb::itch5::reg_sho_restriction_message decoder works
 * as expected.
 */
BOOST_AUTO_TEST_CASE(decode_reg_sho_restriction_message) {
  using namespace jb::itch5;
  using namespace std::chrono;

  auto expected_ts = duration_cast<nanoseconds>(
      hours(11) + minutes(32) + seconds(31) + nanoseconds(123456789L));

  auto x = decoder<true,reg_sho_restriction_message>::r(bufsize, buf, 0);
  BOOST_CHECK_EQUAL(
      x.header.message_type, reg_sho_restriction_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.stock, "HSART");
  BOOST_CHECK_EQUAL(x.reg_sho_action, u'0');

  x = decoder<false,reg_sho_restriction_message>::r(bufsize, buf, 0);
  BOOST_CHECK_EQUAL(
      x.header.message_type, reg_sho_restriction_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.stock, "HSART");
  BOOST_CHECK_EQUAL(x.reg_sho_action, u'0');
}

/**
 * @test Verify that jb::itch5::reg_sho_restriction_message iostream
 * operator works as expected.
 */
BOOST_AUTO_TEST_CASE(stream_reg_sho_restriction_message) {
  using namespace std::chrono;
  using namespace jb::itch5;

  auto tmp = decoder<false,reg_sho_restriction_message>::r(bufsize, buf, 0);
  std::ostringstream os;
  os << tmp;
  BOOST_CHECK_EQUAL(
      os.str(), "message_type=Y,stock_locate=0"
      ",tracking_number=1,timestamp=113231.123456789"
      ",stock=HSART"
      ",reg_sho_action=0"
      );
}

/**
 * @test Verify that reg_sho_action_t works as expected.
 */
BOOST_AUTO_TEST_CASE(simple_reg_sho_action) {
  using namespace jb::itch5;
  BOOST_CHECK_NO_THROW(reg_sho_action_t(u'0'));
  BOOST_CHECK_NO_THROW(reg_sho_action_t(u'1'));
  BOOST_CHECK_NO_THROW(reg_sho_action_t(u'2'));
  BOOST_CHECK_THROW(reg_sho_action_t(u'*'), std::runtime_error);
}
