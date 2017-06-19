#include <jb/pitch2/time_message.hpp>

#include <boost/test/unit_test.hpp>
#include <type_traits>

/**
 * @test Verify that jb::pitch2::time_message works as expected.
 */
BOOST_AUTO_TEST_CASE(time_message_basic) {
  using namespace jb::pitch2;
  BOOST_CHECK_EQUAL(true, std::is_pod<time_message>::value);
  BOOST_CHECK_EQUAL(sizeof(time_message), std::size_t(6));

  char const buf[] = u8"\x06"           // Length (6)
                     "\x20"             // Message Type (32)
                     "\x98\x85\x00\x00" // Time (34200 seconds)
      ;
  time_message msg;
  BOOST_REQUIRE_EQUAL(sizeof(buf) - 1, sizeof(msg));
  std::memcpy(&msg, buf, sizeof(msg));
  BOOST_CHECK_EQUAL(int(msg.length.value()), 6);
  BOOST_CHECK_EQUAL(int(msg.message_type.value()), 0x20);
  BOOST_CHECK_EQUAL(msg.time.value(), 34200);

  std::ostringstream os;
  os << msg;
  BOOST_CHECK_EQUAL(
      os.str(), std::string("length=6,message_type=32,time=34200"));
}
