#include <jb/pitch2/auction_update_message.hpp>

#include <boost/test/unit_test.hpp>
#include <type_traits>

/**
 * @test Verify that jb::pitch::auction_update_message works as expected.
 */
BOOST_AUTO_TEST_CASE(decode_auction_update_message) {
  using namespace jb::pitch2;
  BOOST_CHECK_EQUAL(true, std::is_pod<auction_update_message>::value);

  char const buf[] =
      u8"\x2F"                           // Length (47)
      "\x95"                             // Message Type (149)
      "\x18\xD2\x06\x00"                 // Time Offset (447,000)
      "\x5A\x56\x5A\x5A\x54\x20\x20\x20" // Symbol (ZVZZT)
      "\x49"                             // Auction Type (I = IPO)
      "\xE8\xA3\x0F\x00\x00\x00\x00\x00" // Reference Price ($102.50)
      "\xF8\x24\x01\x00"                 // Buy Side Shares (75,000)
      "\x20\x4E\x00\x00"                 // Sell Side Shares (20,000)
      "\xE9\xA3\x0F\x00\x00\x00\x00\x00" // Indicative Price ($102.5001)
      "\xEA\xA3\x0F\x00\x00\x00\x00\x00" // Auction Only Price ($102.5002)
      ;
  auction_update_message msg;
  BOOST_REQUIRE_EQUAL(sizeof(buf) - 1, sizeof(msg));
  std::memcpy(&msg, buf, sizeof(msg));
  BOOST_CHECK_EQUAL(int(msg.length.value()), 47);
  BOOST_CHECK_EQUAL(int(msg.message_type.value()), 0x95);
  BOOST_CHECK_EQUAL(msg.time_offset.value(), 447000);
  BOOST_CHECK_EQUAL(msg.stock_symbol, jb::fixed_string<8>("ZVZZT"));
  BOOST_CHECK_EQUAL(int(msg.auction_type.value()), 0x49);
  BOOST_CHECK_EQUAL(msg.reference_price.value(), 1025000);
  BOOST_CHECK_EQUAL(msg.buy_shares.value(), 75000);
  BOOST_CHECK_EQUAL(msg.sell_shares.value(), 20000);
  BOOST_CHECK_EQUAL(msg.indicative_price.value(), 1025001);
  BOOST_CHECK_EQUAL(msg.auction_only_price.value(), 1025002);

  std::ostringstream os;
  os << msg;
  BOOST_CHECK_EQUAL(
      os.str(),
      std::string(
          "length=47,message_type=149,time_offset=447000,stock_symbol=ZVZZT   "
          ",auction_type=I,reference_price=1025000,buy_shares=75000"
          ",sell_shares=20000,indicative_price=1025001"
          ",auction_only_price=1025002"));
}
