#include <jb/pitch2/modify_message.hpp>

#include <boost/test/unit_test.hpp>
#include <type_traits>

/**
 * @test Verify that jb::pitch2::modify_long_message works as expected.
 */
BOOST_AUTO_TEST_CASE(modify_message_long_basic) {
  using namespace jb::pitch2;
  BOOST_CHECK_EQUAL(true, std::is_pod<modify_long_message>::value);
  BOOST_CHECK_EQUAL(sizeof(modify_long_message), std::size_t(27));

  char const buf[] = u8"\x1B"           // Length (27)
                     "\x27"             // Message Type (39)
                     "\x18\xD2\x06\x00" // Time Offset (447,000 ns)
                     "\x05\x40\x5B\x77\x8F\x56\x1D\x0B" // Order Id
                     "\xF8\x24\x01\x00"                 // Quantity (75,000)
                     "\xE8\xA3\x0F\x00\x00\x00\x00\x00" // Price ($102.50)
                     "\x03"                             // Modify Flags
      ;
  modify_long_message msg;
  BOOST_REQUIRE_EQUAL(sizeof(buf) - 1, sizeof(msg));
  std::memcpy(&msg, buf, sizeof(msg));
  BOOST_CHECK_EQUAL(int(msg.length.value()), 27);
  BOOST_CHECK_EQUAL(int(msg.message_type.value()), 39);
  BOOST_CHECK_EQUAL(msg.time_offset.value(), 447000);
  BOOST_CHECK_EQUAL(msg.order_id.value(), 0x0B1D568F775B4005ULL);
  BOOST_CHECK_EQUAL(msg.quantity.value(), 75000);
  BOOST_CHECK_EQUAL(msg.price.value(), 1025000);
  BOOST_CHECK_EQUAL(msg.modify_flags.value(), 0x03);

  std::ostringstream os;
  os << msg;
  BOOST_CHECK_EQUAL(
      os.str(), std::string(
                    "length=27,message_type=39,time_offset=447000"
                    ",order_id=800891482924597253,quantity=75000"
                    ",price=1025000,modify_flags=3"));
}

/**
 * @test Verify that jb::pitch2::modify_short_message works as expected.
 */
BOOST_AUTO_TEST_CASE(modify_message_short_basic) {
  using namespace jb::pitch2;
  BOOST_CHECK_EQUAL(true, std::is_pod<modify_short_message>::value);
  BOOST_CHECK_EQUAL(sizeof(modify_short_message), std::size_t(19));

  char const buf[] = u8"\x13"           // Length (19)
                     "\x28"             // Message Type (40)
                     "\x18\xD2\x06\x00" // Time Offset (447,000 ns)
                     "\x05\x40\x5B\x77\x8F\x56\x1D\x0B" // Order Id
                     "\x64\x00"                         // Quantity (100)
                     "\x0A\x28"                         // Price ($102.50)
                     "\x03"                             // Modify Flags
      ;
  modify_short_message msg;
  BOOST_REQUIRE_EQUAL(sizeof(buf) - 1, sizeof(msg));
  std::memcpy(&msg, buf, sizeof(msg));
  BOOST_CHECK_EQUAL(int(msg.length.value()), 19);
  BOOST_CHECK_EQUAL(int(msg.message_type.value()), 40);
  BOOST_CHECK_EQUAL(msg.time_offset.value(), 447000);
  BOOST_CHECK_EQUAL(msg.order_id.value(), 0x0B1D568F775B4005ULL);
  BOOST_CHECK_EQUAL(msg.quantity.value(), 100);
  BOOST_CHECK_EQUAL(msg.price.value(), 10250);
  BOOST_CHECK_EQUAL(msg.modify_flags.value(), 0x03);

  std::ostringstream os;
  os << msg;
  BOOST_CHECK_EQUAL(
      os.str(), std::string(
                    "length=19,message_type=40,time_offset=447000"
                    ",order_id=800891482924597253,quantity=100"
                    ",price=10250,modify_flags=3"));
}
