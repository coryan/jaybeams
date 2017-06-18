#include <jb/pitch2/reduce_size_message.hpp>

#include <boost/test/unit_test.hpp>
#include <type_traits>

/**
 * @test Verify that jb::pitch2::reduce_size_long_message works as expected.
 */
BOOST_AUTO_TEST_CASE(reduce_size_message_long_basic) {
  using namespace jb::pitch2;
  BOOST_CHECK_EQUAL(true, std::is_pod<reduce_size_long_message>::value);
  BOOST_CHECK_EQUAL(sizeof(reduce_size_long_message), std::size_t(18));

  char const buf[] = u8"\x12"           // Length (18)
                     "\x25"             // Message Type (0x25)
                     "\x18\xD2\x06\x00" // Time Offset (447,000 ns)
                     "\x05\x40\x5B\x77\x8F\x56\x1D\x0B" // Order Id
                     "\x64\x00\x00\x00" // Canceled Quantity (100)
      ;
  reduce_size_long_message msg;
  BOOST_REQUIRE_EQUAL(sizeof(buf) - 1, sizeof(msg));
  std::memcpy(&msg, buf, sizeof(msg));
  BOOST_CHECK_EQUAL(int(msg.length.value()), 18);
  BOOST_CHECK_EQUAL(int(msg.message_type.value()), 0x25);
  BOOST_CHECK_EQUAL(msg.time_offset.value(), 447000);
  BOOST_CHECK_EQUAL(msg.order_id.value(), 0x0B1D568F775B4005ULL);
  BOOST_CHECK_EQUAL(msg.canceled_quantity.value(), 100);

  std::ostringstream os;
  os << msg;
  BOOST_CHECK_EQUAL(
      os.str(), std::string(
                    "length=18,message_type=37,time_offset=447000"
                    ",order_id=800891482924597253,canceled_quantity=100"));
}

/**
 * @test Verify that jb::pitch2::reduce_size_short_message works as expected.
 */
BOOST_AUTO_TEST_CASE(reduce_size_message_short_basic) {
  using namespace jb::pitch2;
  BOOST_CHECK_EQUAL(true, std::is_pod<reduce_size_short_message>::value);
  BOOST_CHECK_EQUAL(sizeof(reduce_size_short_message), std::size_t(16));

  char const buf[] = u8"\x10"           // Length (16)
                     "\x26"             // Message Type (0x26)
                     "\x18\xD2\x06\x00" // Time Offset (447,000 ns)
                     "\x05\x40\x5B\x77\x8F\x56\x1D\x0B" // Order Id
                     "\x64\x00" // Canceled Quantity (100)
      ;
  reduce_size_short_message msg;
  BOOST_REQUIRE_EQUAL(sizeof(buf) - 1, sizeof(msg));
  std::memcpy(&msg, buf, sizeof(msg));
  BOOST_CHECK_EQUAL(int(msg.length.value()), 16);
  BOOST_CHECK_EQUAL(int(msg.message_type.value()), 0x26);
  BOOST_CHECK_EQUAL(msg.time_offset.value(), 447000);
  BOOST_CHECK_EQUAL(msg.order_id.value(), 0x0B1D568F775B4005ULL);
  BOOST_CHECK_EQUAL(msg.canceled_quantity.value(), 100);

  std::ostringstream os;
  os << msg;
  BOOST_CHECK_EQUAL(
      os.str(), std::string(
                    "length=16,message_type=38,time_offset=447000"
                    ",order_id=800891482924597253,canceled_quantity=100"));
}
