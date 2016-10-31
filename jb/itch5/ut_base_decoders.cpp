#include <jb/itch5/base_decoders.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that base decoders work as expected.
 */
BOOST_AUTO_TEST_CASE(decode_uint8) {
  using jb::itch5::decoder;
  // The actual buffer size is 32 bytes, but we treat them as 16 byte
  // buffers.  This is because some of the tests verify we (a) do
  // detect attempts to read past the end of the array, and (b) when
  // checking is disabled, we do allow reading past the 'end'.
  // Using a larger buffer allows such tests without involving
  // undefined behavior.
  char buffer[32];
  buffer[1] = 20;
  buffer[3] = 25;

  auto actual = decoder<true, std::uint8_t>::r(16, buffer, 1);
  BOOST_CHECK_EQUAL(actual, 20);

  actual = decoder<false, std::uint8_t>::r(16, buffer, 3);
  BOOST_CHECK_EQUAL(actual, 25);

  BOOST_CHECK_NO_THROW((decoder<true, std::uint8_t>::r(16, buffer, 0)));
  BOOST_CHECK_NO_THROW((decoder<true, std::uint8_t>::r(16, buffer, 8)));
  BOOST_CHECK_NO_THROW((decoder<true, std::uint8_t>::r(16, buffer, 15)));
  BOOST_CHECK_THROW(
      (decoder<true, std::uint8_t>::r(16, buffer, 16)), std::runtime_error);
  BOOST_CHECK_NO_THROW((decoder<false, std::uint8_t>::r(16, buffer, 16)));
}

/**
 * @test Verify that base decoders work as expected.
 */
BOOST_AUTO_TEST_CASE(decode_uint16) {
  using jb::itch5::decoder;
  char buffer[32];
  buffer[0] = 10;
  buffer[1] = 20;

  auto actual = decoder<true, std::uint16_t>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual, 10 * 256 + 20);

  actual = decoder<false, std::uint16_t>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual, 10 * 256 + 20);

  BOOST_CHECK_NO_THROW((decoder<true, std::uint16_t>::r(16, buffer, 0)));
  BOOST_CHECK_NO_THROW((decoder<true, std::uint16_t>::r(16, buffer, 8)));
  BOOST_CHECK_NO_THROW((decoder<true, std::uint16_t>::r(16, buffer, 14)));
  BOOST_CHECK_THROW(
      (decoder<true, std::uint16_t>::r(16, buffer, 15)), std::runtime_error);
  BOOST_CHECK_NO_THROW((decoder<false, std::uint16_t>::r(16, buffer, 15)));
}

/**
 * @test Verify that base decoders work as expected.
 */
BOOST_AUTO_TEST_CASE(decode_uint32) {
  using jb::itch5::decoder;
  char buffer[32];
  buffer[0] = 10;
  buffer[1] = 20;
  buffer[2] = 30;
  buffer[3] = 40;

  auto actual = decoder<true, std::uint32_t>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual, ((10 * 256 + 20) * 256 + 30) * 256 + 40);

  actual = decoder<false, std::uint32_t>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual, ((10 * 256 + 20) * 256 + 30) * 256 + 40);

  BOOST_CHECK_NO_THROW((decoder<true, std::uint32_t>::r(16, buffer, 0)));
  BOOST_CHECK_NO_THROW((decoder<true, std::uint32_t>::r(16, buffer, 8)));
  BOOST_CHECK_NO_THROW((decoder<true, std::uint32_t>::r(16, buffer, 12)));
  BOOST_CHECK_THROW(
      (decoder<true, std::uint32_t>::r(16, buffer, 13)), std::runtime_error);
  BOOST_CHECK_NO_THROW((decoder<false, std::uint32_t>::r(16, buffer, 13)));
}

/**
 * @test Verify that base decoders work as expected.
 */
BOOST_AUTO_TEST_CASE(decode_uint64) {
  using jb::itch5::decoder;
  char buffer[32];
  int values[] = {10, 20, 30, 40, 15, 25, 35, 45};
  std::uint64_t expected = 0;
  for (int i = 0; i != 8; ++i) {
    buffer[i] = values[i];
    expected = expected * 256 + values[i];
  }

  auto actual = decoder<true, std::uint64_t>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual, expected);

  actual = decoder<false, std::uint64_t>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual, expected);

  BOOST_CHECK_NO_THROW((decoder<true, std::uint64_t>::r(16, buffer, 2)));
  BOOST_CHECK_NO_THROW((decoder<true, std::uint64_t>::r(16, buffer, 7)));
  BOOST_CHECK_THROW(
      (decoder<true, std::uint64_t>::r(16, buffer, 9)), std::runtime_error);
  BOOST_CHECK_NO_THROW((decoder<false, std::uint64_t>::r(16, buffer, 9)));
}
