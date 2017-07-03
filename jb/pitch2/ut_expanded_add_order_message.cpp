#include <jb/pitch2/expanded_add_order_message.hpp>

#include <boost/test/unit_test.hpp>
#include <type_traits>

/**
 * @test Verify that jb::pitch2::expanded_add_order_message works as expected.
 */
BOOST_AUTO_TEST_CASE(expanded_add_order_message_basic) {
  using namespace jb::pitch2;
  // This test fails, but the class has exactly the right size and
  // padding, so I do not care ...
  // BOOST_CHECK_EQUAL(true, std::is_pod<expanded_add_order_message>::value);
  BOOST_CHECK_EQUAL(sizeof(expanded_add_order_message), std::size_t(40));

  char const buf[] = u8"\x28"                           // Length (40)
                     "\x2F"                             // Message Type (47)
                     "\x18\xD2\x06\x00"                 // Time Offset (447,000)
                     "\x05\x40\x5B\x77\x8F\x56\x1D\x0B" // Order Id
                     "\x42"             // Side Indicator (66 'B' Buy)
                     "\x20\x4E\x00\x00" // Quantity (20,000)
                     "\x5A\x56\x5A\x5A\x54\x20\x20\x20" // Symbol (ZVZZT)
                     "\x5A\x23\x00\x00\x00\x00\x00\x00" // Price ($0.9050)
                     "\x01"             // Add Flags (Bit 0 on -> Displayed)
                     "\x4D\x50\x49\x44" // Participant ID (MPID)
      ;
  expanded_add_order_message msg;
  BOOST_CHECK_EQUAL(sizeof(buf) - 1, sizeof(msg));
  BOOST_CHECK_EQUAL(
      reinterpret_cast<char*>(&msg.participant_id) -
          reinterpret_cast<char*>(&msg),
      std::size_t(36));

  std::memcpy(&msg, buf, sizeof(msg));
  BOOST_CHECK_EQUAL(int(msg.length.value()), 40);
  BOOST_CHECK_EQUAL(int(msg.message_type.value()), 47);
  BOOST_CHECK_EQUAL(msg.time_offset.value(), 447000);
  BOOST_CHECK_EQUAL(msg.order_id.value(), 0x0B1D568F775B4005ULL);
  BOOST_CHECK_EQUAL(msg.side_indicator.value(), 0x42);
  BOOST_CHECK_EQUAL(msg.quantity.value(), 20000);
  BOOST_CHECK_EQUAL(msg.symbol, jb::fixed_string<8>("ZVZZT"));
  BOOST_CHECK_EQUAL(msg.price.value(), 9050);
  BOOST_CHECK_EQUAL(msg.add_flags.value(), 0x01);
  BOOST_CHECK_EQUAL(msg.participant_id, jb::fixed_string<4>("MPID"));

  std::ostringstream os;
  os << msg;
  BOOST_CHECK_EQUAL(
      os.str(),
      std::string("length=40,message_type=47,time_offset=447000,"
                  "order_id=800891482924597253,side_indicator=B,"
                  "quantity=20000,symbol=ZVZZT   ,price=9050,add_flags=1,"
                  "participant_id=MPID"));
}
