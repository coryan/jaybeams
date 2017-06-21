#include <jb/pitch2/delete_order_message.hpp>

#include <boost/test/unit_test.hpp>
#include <type_traits>

/**
 * @test Verify that jb::pitch2::delete_order_message works as expected.
 */
BOOST_AUTO_TEST_CASE(delete_order_message_basic) {
  using namespace jb::pitch2;
  BOOST_CHECK_EQUAL(true, std::is_pod<delete_order_message>::value);
  BOOST_CHECK_EQUAL(sizeof(delete_order_message), std::size_t(14));

  char const buf[] = u8"\x0E"                           // Length (14)
                     "\x29"                             // Message Type (41)
                     "\x18\xD2\x06\x00"                 // Time Offset (447,000)
                     "\x05\x40\x5B\x77\x8F\x56\x1D\x0B" // Order Id
      ;
  delete_order_message msg;
  BOOST_REQUIRE_EQUAL(sizeof(buf) - 1, sizeof(msg));
  std::memcpy(&msg, buf, sizeof(msg));
  BOOST_CHECK_EQUAL(int(msg.length.value()), 14);
  BOOST_CHECK_EQUAL(int(msg.message_type.value()), 41);
  BOOST_CHECK_EQUAL(msg.time_offset.value(), 447000);
  BOOST_CHECK_EQUAL(msg.order_id.value(), 0x0B1D568F775B4005ULL);

  std::ostringstream os;
  os << msg;
  BOOST_CHECK_EQUAL(
      os.str(), std::string("length=14,message_type=41,time_offset=447000,"
                            "order_id=800891482924597253"));
}
