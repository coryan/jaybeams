#include <jb/itch5/system_event_message.hpp>
#include <jb/itch5/testing/data.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::itch5::system_event_message decoder works as expected.
 */
BOOST_AUTO_TEST_CASE(decode_system_event_message) {
  using jb::itch5::system_event_message;
  using jb::itch5::decoder;
  using namespace std::chrono;

  auto buf = jb::itch5::testing::system_event();
  auto expected_ts = jb::itch5::testing::expected_ts();

  auto x = decoder<true, system_event_message>::r(buf.second, buf.first, 0);
  BOOST_CHECK_EQUAL(x.header.message_type, system_event_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.event_code, u'O');

  x = decoder<false, system_event_message>::r(buf.second, buf.first, 0);
  BOOST_CHECK_EQUAL(x.header.message_type, system_event_message::message_type);
  BOOST_CHECK_EQUAL(x.header.stock_locate, 0);
  BOOST_CHECK_EQUAL(x.header.tracking_number, 1);
  BOOST_CHECK_EQUAL(x.header.timestamp.ts.count(), expected_ts.count());
  BOOST_CHECK_EQUAL(x.event_code, u'O');
}

/**
 * @test Verify that jb::itch5::system_event_message iostream operator works
 * as expected.
 */
BOOST_AUTO_TEST_CASE(stream_system_event_message) {
  using namespace std::chrono;
  using namespace jb::itch5;

  auto ts = timestamp{duration_cast<nanoseconds>(
      hours(11) + minutes(32) + seconds(31) + nanoseconds(123456789L))};

  system_event_message tmp{message_header{u' ', 0, 1, ts}, event_code_t(u'O')};
  std::ostringstream os;
  os << tmp;
  BOOST_CHECK_EQUAL(
      os.str(), "message_type= ,stock_locate=0,"
                "tracking_number=1,timestamp=113231.123456789"
                ",event_code=O");
}

/**
 * @test Verify that event_code_t works as expected.
 */
BOOST_AUTO_TEST_CASE(simple_event_code) {
  using namespace jb::itch5;
  BOOST_CHECK_NO_THROW(event_code_t(u'O'));
  BOOST_CHECK_NO_THROW(event_code_t(u'S'));
  BOOST_CHECK_NO_THROW(event_code_t(u'Q'));
  BOOST_CHECK_NO_THROW(event_code_t(u'M'));
  BOOST_CHECK_NO_THROW(event_code_t(u'E'));
  BOOST_CHECK_NO_THROW(event_code_t(u'C'));
  BOOST_CHECK_THROW(event_code_t(u'*'), std::runtime_error);
}
