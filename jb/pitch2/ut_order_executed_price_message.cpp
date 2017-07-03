#include <jb/pitch2/order_executed_price_message.hpp>

#include <boost/test/unit_test.hpp>
#include <type_traits>

/**
 * @test Verify that jb::pitch2::order_executed_price_message works as expected.
 */
BOOST_AUTO_TEST_CASE(order_executed_price_message_basic) {
  using namespace jb::pitch2;
  BOOST_CHECK_EQUAL(true, std::is_pod<order_executed_price_message>::value);
  BOOST_CHECK_EQUAL(sizeof(order_executed_price_message), std::size_t(38));

  char const buf[] = u8"\x26"           // Length (38)
                     "\x24"             // Message Type (0x24)
                     "\x18\xD2\x06\x00" // Time Offset (447,000 ns)
                     "\x05\x40\x5B\x77\x8F\x56\x1D\x0B" // Order Id
                     "\x64\x00\x00\x00" // Executed Quantity (100)
                     "\xBC\x4D\x00\x00" // Remaining Quantity (19,900)
                     "\x34\x2B\x46\xE0\xBB\x00\x00\x00" // Execution Id
                     "\xE8\xA3\x0F\x00\x00\x00\x00\x00" // Price ($102.50)
      ;
  order_executed_price_message msg;
  BOOST_REQUIRE_EQUAL(sizeof(buf) - 1, sizeof(msg));
  std::memcpy(&msg, buf, sizeof(msg));
  BOOST_CHECK_EQUAL(int(msg.length.value()), 38);
  BOOST_CHECK_EQUAL(int(msg.message_type.value()), 0x24);
  BOOST_CHECK_EQUAL(msg.time_offset.value(), 447000);
  BOOST_CHECK_EQUAL(msg.order_id.value(), 0x0B1D568F775B4005ULL);
  BOOST_CHECK_EQUAL(msg.executed_quantity.value(), 100);
  BOOST_CHECK_EQUAL(msg.remaining_quantity.value(), 19900);
  BOOST_CHECK_EQUAL(msg.execution_id.value(), 0x000000BBE0462B34ULL);
  BOOST_CHECK_EQUAL(msg.price.value(), 1025000);

  std::ostringstream os;
  os << msg;
  BOOST_CHECK_EQUAL(
      os.str(),
      std::string("length=38,message_type=36,time_offset=447000"
                  ",order_id=800891482924597253,executed_quantity=100"
                  ",remaining_quantity=19900,execution_id=806921579316"
                  ",price=1025000"));
}
