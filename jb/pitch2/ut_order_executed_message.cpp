#include <jb/pitch2/order_executed_message.hpp>

#include <boost/test/unit_test.hpp>
#include <type_traits>

/**
 * @test Verify that jb::pitch2::order_executed_message works as expected.
 */
BOOST_AUTO_TEST_CASE(order_executed_message_basic) {
  using namespace jb::pitch2;
  BOOST_CHECK_EQUAL(true, std::is_pod<order_executed_message>::value);
  BOOST_CHECK_EQUAL(sizeof(order_executed_message), std::size_t(26));

  char const buf[] = u8"\x1A"           // Length (26)
                     "\x23"             // Message Type (0x23)
                     "\x18\xD2\x06\x00" // Time Offset (447,000 ns)
                     "\x05\x40\x5B\x77\x8F\x56\x1D\x0B" // Order Id
                     "\x64\x00\x00\x00" // Executed Quantity (100)
                     "\x34\x2B\x46\xE0\xBB\x00\x00\x00" // Execution Id
      ;
  order_executed_message msg;
  BOOST_REQUIRE_EQUAL(sizeof(buf) - 1, sizeof(msg));
  std::memcpy(&msg, buf, sizeof(msg));
  BOOST_CHECK_EQUAL(int(msg.length.value()), 26);
  BOOST_CHECK_EQUAL(int(msg.message_type.value()), 0x23);
  BOOST_CHECK_EQUAL(msg.time_offset.value(), 447000);
  BOOST_CHECK_EQUAL(msg.order_id.value(), 0x0B1D568F775B4005ULL);
  BOOST_CHECK_EQUAL(msg.executed_quantity.value(), 100);
  BOOST_CHECK_EQUAL(msg.execution_id.value(), 0x000000BBE0462B34ULL);

  std::ostringstream os;
  os << msg;
  BOOST_CHECK_EQUAL(
      os.str(), std::string(
                    "length=26,message_type=35,time_offset=447000"
                    ",order_id=800891482924597253,executed_quantity=100"
                    ",execution_id=806921579316"));
}
