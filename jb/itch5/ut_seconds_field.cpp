#include <jb/itch5/seconds_field.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::itch5::decoder works for jb::itch5::seconds_field
 * as expected.
 */
BOOST_AUTO_TEST_CASE(decode_seconds_field) {
  using jb::itch5::seconds_field;
  using jb::itch5::decoder;
  using namespace std::chrono;

  char buffer[32];
  std::memset(buffer, 0, sizeof(buffer));
  std::memcpy(buffer, "\x00\x00\x64\x59", 4); // 07:08:09

  auto expected = duration_cast<seconds>(
      hours(7) + minutes(8) + seconds(9)).count();

  auto actual = decoder<true,seconds_field>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual.int_seconds(), expected);

  actual = decoder<false,seconds_field>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual.int_seconds(), expected);

  // In the following tests we are simply checking the range, so zero
  // out the buffer to avoid range errors due to uninitialized memory.
  std::memset(buffer, 0, sizeof(buffer));
  BOOST_CHECK_NO_THROW((decoder<true,seconds_field>::r(16, buffer, 2)));
  BOOST_CHECK_NO_THROW((decoder<true,seconds_field>::r(16, buffer, 12)));
  BOOST_CHECK_THROW(
      (decoder<true,seconds_field>::r(16, buffer, 13)), std::runtime_error);
  BOOST_CHECK_NO_THROW((decoder<false,seconds_field>::r(16, buffer, 13)));
}

/**
 * @test Verify that the jb::itch5::decoder detects out of range
 * errors for jb::itch5::seconds_field.
 */
BOOST_AUTO_TEST_CASE(decode_seconds_field_range) {
  using jb::itch5::seconds_field;
  using jb::itch5::decoder;
  char buffer[32];

  std::memset(buffer, 0, sizeof(buffer));
  std::memcpy(buffer, "\x00\x01\x51\x80", 4); // 24:00:00

  BOOST_CHECK_THROW(
      (decoder<true,seconds_field>::r(16, buffer, 0)), std::runtime_error);
  BOOST_CHECK_NO_THROW(
      (decoder<false,seconds_field>::r(16, buffer, 0)));
}

/**
 * @test Verify that jb::itch5::seconds_field iostream operator works
 * as expected.
 */
BOOST_AUTO_TEST_CASE(stream_seconds_field) {
  using namespace std::chrono;
  using jb::itch5::seconds_field;

  {  
    auto nn = duration_cast<seconds>(hours(7) + minutes(8) + seconds(9));
    std::ostringstream os;
    os << seconds_field(nn);
    BOOST_CHECK_EQUAL(os.str(), "07:08:09");
  }

  {
    auto nn = duration_cast<seconds>(hours(9) + minutes(30) + seconds(0));
    std::ostringstream os;
    os << seconds_field{nn};
    BOOST_CHECK_EQUAL(os.str(), "09:30:00");
  }

  {
    auto nn = duration_cast<seconds>(hours(15) + minutes(59) + seconds(59));
    std::ostringstream os;
    os << seconds_field{nn};
    BOOST_CHECK_EQUAL(os.str(), "15:59:59");
  }

  {
    auto nn = duration_cast<seconds>(hours(16) + minutes(0) + seconds(0));
    std::ostringstream os;
    os << seconds_field{nn};
    BOOST_CHECK_EQUAL(os.str(), "16:00:00");
  }
}
