#include <jb/pitch2/unit_clear_message.hpp>

#include <boost/test/unit_test.hpp>
#include <type_traits>

/**
 * @test Verify that jb::pitch2::unit_clear_message works as expected.
 */
BOOST_AUTO_TEST_CASE(unit_clear_message_basic) {
  using namespace jb::pitch2;
  BOOST_CHECK_EQUAL(true, std::is_pod<unit_clear_message>::value);
  BOOST_CHECK_EQUAL(sizeof(unit_clear_message), std::size_t(6));

  char const buf[] = u8"\x06"           // Length (6)
                     "\x97"             // Message Type (0x97)
                     "\x18\xD2\x06\x00" // Time Offset (447,000 ns)
      ;
  unit_clear_message msg;
  BOOST_REQUIRE_EQUAL(sizeof(buf) - 1, sizeof(msg));
  std::memcpy(&msg, buf, sizeof(msg));
  BOOST_CHECK_EQUAL(int(msg.length.value()), 6);
  BOOST_CHECK_EQUAL(int(msg.message_type.value()), 0x97);
  BOOST_CHECK_EQUAL(msg.time_offset.value(), 447000);

  std::ostringstream os;
  os << msg;
  BOOST_CHECK_EQUAL(
      os.str(), std::string("length=6,message_type=151,time_offset=447000"));
}
