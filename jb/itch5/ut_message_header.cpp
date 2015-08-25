#include <jb/itch5/message_header.hpp>
#include <jb/itch5/testing_data.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::itch5::message_header decoder works as expected.
 */
BOOST_AUTO_TEST_CASE(decode_message_header) {
  using jb::itch5::message_header;
  using jb::itch5::decoder;
  using namespace std::chrono;

  char buf[] =
      u8" "                 // Message Type (space is invalid)
      JB_ITCH5_TEST_HEADER  // Common header body
      ;
  std::size_t const size = sizeof(buf)/sizeof(buf[0]);

  auto expected_ts = duration_cast<nanoseconds>(
      hours(11) + minutes(32) + seconds(31) + nanoseconds(123456789L));

  auto x = decoder<true,message_header>::r(size, buf, 0);
  BOOST_CHECK_EQUAL(x.message_type, u' ');
  BOOST_CHECK_EQUAL(x.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.timestamp.ts.count(), expected_ts.count());

  x = decoder<false,message_header>::r(size, buf, 0);
  BOOST_CHECK_EQUAL(x.message_type, u' ');
  BOOST_CHECK_EQUAL(x.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.timestamp.ts.count(), expected_ts.count());
}

/**
 * @test Verify that jb::itch5::message_header iostream operator works
 * as expected.
 */
BOOST_AUTO_TEST_CASE(stream_message_header) {
  using namespace std::chrono;
  using jb::itch5::message_header;

  auto ts = jb::itch5::timestamp{duration_cast<nanoseconds>(
      hours(11) + minutes(32) + seconds(31) + nanoseconds(123456789L))};

  {
    message_header tmp{u' ', 0, 1, ts};
    std::ostringstream os;
    os << tmp;
    BOOST_CHECK_EQUAL(
        os.str(), "message_type= ,stock_locate=0,"
        "tracking_number=1,timestamp=113231.123456789");
  }

  {
    message_header tmp{255, 0, 1, ts};
    std::ostringstream os;
    os << tmp;
    BOOST_CHECK_EQUAL(
        os.str(), "message_type=.(255),stock_locate=0,"
        "tracking_number=1,timestamp=113231.123456789");
  }
}
