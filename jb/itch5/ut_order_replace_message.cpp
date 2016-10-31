#include <jb/itch5/order_replace_message.hpp>
#include <jb/itch5/testing_data.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that the jb::itch5::order_replace_message decoder works
 * as expected.
 */
BOOST_AUTO_TEST_CASE(decode_order_replace_message) {
  using namespace jb::itch5;
  using namespace std::chrono;

  auto buf = jb::itch5::testing::order_replace();
  auto expected_ts = jb::itch5::testing::expected_ts();

  auto x = decoder<true, order_replace_message>::r(buf.second, buf.first, 0);
  BOOST_CHECK_EQUAL(x.header.message_type, order_replace_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.original_order_reference_number, 42ULL);
  BOOST_CHECK_EQUAL(x.new_order_reference_number, 4242ULL);
  BOOST_CHECK_EQUAL(x.shares, 100);
  BOOST_CHECK_EQUAL(x.price, price4_t(2340600));

  x = decoder<false, order_replace_message>::r(buf.second, buf.first, 0);
  BOOST_CHECK_EQUAL(x.header.message_type, order_replace_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.original_order_reference_number, 42ULL);
  BOOST_CHECK_EQUAL(x.new_order_reference_number, 4242ULL);
  BOOST_CHECK_EQUAL(x.shares, 100);
  BOOST_CHECK_EQUAL(x.price, price4_t(2340600));
}

/**
 * @test Verify that jb::itch5::order_replace_message iostream
 * operator works as expected.
 */
BOOST_AUTO_TEST_CASE(stream_order_replace_message) {
  using namespace std::chrono;
  using namespace jb::itch5;

  auto buf = jb::itch5::testing::order_replace();
  auto tmp = decoder<false, order_replace_message>::r(buf.second, buf.first, 0);
  std::ostringstream os;
  os << tmp;
  BOOST_CHECK_EQUAL(
      os.str(), "message_type=U,stock_locate=0"
                ",tracking_number=1,timestamp=113231.123456789"
                ",original_order_reference_number=42"
                ",new_order_reference_number=4242"
                ",shares=100"
                ",price=234.0600");
}
