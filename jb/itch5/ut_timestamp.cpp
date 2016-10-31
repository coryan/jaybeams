#include <jb/itch5/timestamp.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::itch5::decoder works for jb::itch5::timestamp
 * as expected.
 */
BOOST_AUTO_TEST_CASE(decode_timestamp) {
  using jb::itch5::timestamp;
  using jb::itch5::decoder;
  char buffer[32];
  int values[] = {10, 20, 30, 40, 15, 25};
  std::uint64_t expected = 0;
  for (int i = 0; i != 6; ++i) {
    buffer[i] = values[i];
    expected = expected * 256 + values[i];
  }

  auto actual = decoder<true, timestamp>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual.ts.count(), expected);

  actual = decoder<false, timestamp>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual.ts.count(), expected);

  // In the following tests we are simply checking the range, so zero
  // out the buffer to avoid range errors due to uninitialized memory.
  std::memset(buffer, 0, sizeof(buffer));
  BOOST_CHECK_NO_THROW((decoder<true, timestamp>::r(16, buffer, 2)));
  BOOST_CHECK_NO_THROW((decoder<true, timestamp>::r(16, buffer, 10)));
  BOOST_CHECK_THROW(
      (decoder<true, timestamp>::r(16, buffer, 11)), std::runtime_error);
  BOOST_CHECK_NO_THROW((decoder<false, timestamp>::r(16, buffer, 11)));
}

/**
 * @test Verify that the jb::itch5::decoder detects out of range
 * errors for jb::itch5::timestamp.
 */
BOOST_AUTO_TEST_CASE(decode_timestamp_range) {
  using jb::itch5::timestamp;
  using jb::itch5::decoder;
  char buffer[32];
  int values[] = {255, 255, 255, 255, 255, 255};
  std::uint64_t expected = 0;
  for (int i = 0; i != 6; ++i) {
    buffer[i] = values[i];
    expected = expected * 256 + values[i];
  }

  BOOST_CHECK_THROW(
      (decoder<true, timestamp>::r(16, buffer, 0)), std::runtime_error);
  BOOST_CHECK_NO_THROW((decoder<false, timestamp>::r(16, buffer, 0)));
}

/**
 * @test Verify that jb::itch5::timestamp iostream operator works as expected.
 */
BOOST_AUTO_TEST_CASE(stream_timestamp) {
  using namespace std::chrono;
  using jb::itch5::timestamp;

  {
    auto nn =
        (duration_cast<nanoseconds>(hours(7)) +
         duration_cast<nanoseconds>(minutes(8)) +
         duration_cast<nanoseconds>(seconds(9)) + nanoseconds(20));
    std::ostringstream os;
    os << timestamp{nn};
    BOOST_CHECK_EQUAL(os.str(), "070809.000000020");
  }

  {
    auto nn =
        (duration_cast<nanoseconds>(hours(9)) +
         duration_cast<nanoseconds>(minutes(30)) +
         duration_cast<nanoseconds>(seconds(0)) + nanoseconds(0));
    std::ostringstream os;
    os << timestamp{nn};
    BOOST_CHECK_EQUAL(os.str(), "093000.000000000");
  }

  {
    auto nn =
        (duration_cast<nanoseconds>(hours(15)) +
         duration_cast<nanoseconds>(minutes(59)) +
         duration_cast<nanoseconds>(seconds(59)) + nanoseconds(999999999));
    std::ostringstream os;
    os << timestamp{nn};
    BOOST_CHECK_EQUAL(os.str(), "155959.999999999");
  }

  {
    auto nn =
        (duration_cast<nanoseconds>(hours(16)) +
         duration_cast<nanoseconds>(minutes(0)) +
         duration_cast<nanoseconds>(seconds(0)) + nanoseconds(0));
    std::ostringstream os;
    os << timestamp{nn};
    BOOST_CHECK_EQUAL(os.str(), "160000.000000000");
  }
}

/**
 * @test Verify that jb::itch5::encoder works for jb::itch5::timestamp
 * as expected.
 */
BOOST_AUTO_TEST_CASE(encode_timestamp) {
  using jb::itch5::timestamp;
  using jb::itch5::decoder;
  using jb::itch5::encoder;

  using namespace std::chrono;
  timestamp expected{hours(9) + minutes(31) + seconds(10) + nanoseconds(1234)};

  char buffer[32];
  encoder<true, timestamp>::w(16, buffer, 0, expected);
  auto actual = decoder<true, timestamp>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual.ts.count(), expected.ts.count());

  encoder<false, timestamp>::w(16, buffer, 0, expected);
  actual = decoder<false, timestamp>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual.ts.count(), expected.ts.count());

  timestamp ts{seconds(100)};
  BOOST_CHECK_NO_THROW((encoder<true, timestamp>::w(16, buffer, 2, ts)));
  BOOST_CHECK_NO_THROW((encoder<true, timestamp>::w(16, buffer, 10, ts)));
  BOOST_CHECK_THROW(
      (encoder<true, timestamp>::w(16, buffer, 11, ts)), std::runtime_error);
  BOOST_CHECK_NO_THROW((encoder<false, timestamp>::w(16, buffer, 11, ts)));
}

/**
 * @test Verify that the jb::itch5::decoder detects out of range
 * errors for jb::itch5::timestamp.
 */
BOOST_AUTO_TEST_CASE(encode_timestamp_range) {
  using jb::itch5::timestamp;
  using jb::itch5::encoder;
  char buffer[32];

  timestamp ts{std::chrono::hours(48)};
  BOOST_CHECK_THROW(
      (encoder<true, timestamp>::w(16, buffer, 0, ts)), std::runtime_error);
  BOOST_CHECK_NO_THROW((encoder<false, timestamp>::w(16, buffer, 0, ts)));
}
