#include <jb/itch5/base_encoders.hpp>
#include <jb/itch5/base_decoders.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that base encoders work as expected.
 */
BOOST_AUTO_TEST_CASE(encode_uint8) {
  using jb::itch5::encoder;
  using jb::itch5::decoder;
  // The actual buffer size is 32 bytes, but we treat them as 16 byte
  // buffers.  This is because some of the tests verify we (a) do
  // detect attempts to read past the end of the array, and (b) when
  // checking is disabled, we do allow reading past the 'end'.
  // Using a larger buffer allows such tests without involving
  // undefined behavior.
  char buffer[32] = {0};

  encoder<true, std::uint8_t>::w(16, buffer, 1, std::uint8_t(20));
  auto actual = decoder<true, std::uint8_t>::r(16, buffer, 1);
  BOOST_CHECK_EQUAL(actual, 20);
  BOOST_CHECK_EQUAL(buffer[1], 20);

  encoder<true, std::uint8_t>::w(16, buffer, 3, std::uint8_t(25));
  actual = decoder<false, std::uint8_t>::r(16, buffer, 3);
  BOOST_CHECK_EQUAL(actual, 25);
  BOOST_CHECK_EQUAL(buffer[3], 25);

  BOOST_CHECK_NO_THROW((encoder<true, std::uint8_t>::w(16, buffer, 0, 0)));
  BOOST_CHECK_NO_THROW((encoder<true, std::uint8_t>::w(16, buffer, 8, 0)));
  BOOST_CHECK_NO_THROW((encoder<true, std::uint8_t>::w(16, buffer, 15, 0)));
  BOOST_CHECK_THROW((encoder<true, std::uint8_t>::w(16, buffer, 16, 0)),
                    std::runtime_error);
  BOOST_CHECK_NO_THROW((encoder<false, std::uint8_t>::w(16, buffer, 16, 0)));
}

/**
 * @test Verify that base encoders work as expected.
 */
BOOST_AUTO_TEST_CASE(encode_uint16) {
  using jb::itch5::encoder;
  using jb::itch5::decoder;
  char buffer[32];

  std::uint16_t expected = 0xAA10;
  encoder<true, std::uint16_t>::w(16, buffer, 0, expected);
  auto actual = decoder<true, std::uint16_t>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual, expected);
  BOOST_CHECK_EQUAL(std::uint8_t(buffer[0]), 0xAA);
  BOOST_CHECK_EQUAL(std::uint8_t(buffer[1]), 0x10);

  encoder<false, std::uint16_t>::w(16, buffer, 0, expected);
  actual = decoder<false, std::uint16_t>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual, expected);
  BOOST_CHECK_EQUAL(std::uint8_t(buffer[0]), 0xAA);
  BOOST_CHECK_EQUAL(std::uint8_t(buffer[1]), 0x10);

  BOOST_CHECK_NO_THROW((encoder<true, std::uint16_t>::w(16, buffer, 0, 0)));
  BOOST_CHECK_NO_THROW((encoder<true, std::uint16_t>::w(16, buffer, 8, 0)));
  BOOST_CHECK_NO_THROW((encoder<true, std::uint16_t>::w(16, buffer, 14, 0)));
  BOOST_CHECK_THROW((encoder<true, std::uint16_t>::w(16, buffer, 15, 0)),
                    std::runtime_error);
  BOOST_CHECK_NO_THROW((encoder<false, std::uint16_t>::w(16, buffer, 15, 0)));
}

/**
 * @test Verify that base encoders work as expected.
 */
BOOST_AUTO_TEST_CASE(encode_uint32) {
  using jb::itch5::encoder;
  using jb::itch5::decoder;
  char buffer[32];
  std::uint32_t expected = 0x10203040;

  unsigned char const contents[] = u8"\x10\x20\x30\x40";
  std::size_t const len = sizeof(contents) / sizeof(contents[0]) - 1;

  encoder<true, std::uint32_t>::w(16, buffer, 0, expected);
  auto actual = decoder<true, std::uint32_t>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual, expected);
  BOOST_CHECK_EQUAL_COLLECTIONS(buffer, buffer + 4, contents, contents + len);

  encoder<false, std::uint32_t>::w(16, buffer, 0, expected);
  actual = decoder<false, std::uint32_t>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual, expected);
  BOOST_CHECK_EQUAL_COLLECTIONS(buffer, buffer + 4, contents, contents + len);

  BOOST_CHECK_NO_THROW((encoder<true, std::uint32_t>::w(16, buffer, 0, 0)));
  BOOST_CHECK_NO_THROW((encoder<true, std::uint32_t>::w(16, buffer, 8, 0)));
  BOOST_CHECK_NO_THROW((encoder<true, std::uint32_t>::w(16, buffer, 12, 0)));
  BOOST_CHECK_THROW((encoder<true, std::uint32_t>::w(16, buffer, 13, 0)),
                    std::runtime_error);
  BOOST_CHECK_NO_THROW((encoder<false, std::uint32_t>::w(16, buffer, 13, 0)));
}

/**
 * @test Verify that base encoders work as expected.
 */
BOOST_AUTO_TEST_CASE(encode_uint64) {
  using jb::itch5::encoder;
  using jb::itch5::decoder;
  unsigned char buffer[32];
  std::uint64_t expected = 0xAABBCCDDEEFF0011ULL;
  encoder<true, std::uint64_t>::w(16, buffer, 0, expected);

  unsigned char const contents[] = u8"\xAA\xBB\xCC\xDD\xEE\xFF\x00\x11";
  std::size_t const len = sizeof(contents) / sizeof(contents[0]) - 1;

  auto actual = decoder<true, std::uint64_t>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual, expected);
  BOOST_CHECK_EQUAL_COLLECTIONS(buffer, buffer + 8, contents, contents + len);

  encoder<false, std::uint64_t>::w(16, buffer, 0, expected);
  actual = decoder<false, std::uint64_t>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual, expected);
  BOOST_CHECK_EQUAL_COLLECTIONS(buffer, buffer + 8, contents, contents + len);

  BOOST_CHECK_NO_THROW((encoder<true, std::uint64_t>::w(16, buffer, 2, 0)));
  BOOST_CHECK_NO_THROW((encoder<true, std::uint64_t>::w(16, buffer, 7, 0)));
  BOOST_CHECK_THROW((encoder<true, std::uint64_t>::w(16, buffer, 9, 0)),
                    std::runtime_error);
  BOOST_CHECK_NO_THROW((encoder<false, std::uint64_t>::w(16, buffer, 9, 0)));
}
