#include <jb/itch5/price_field.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::itch5::decoder for jb::itch5::price_field
 * works as expected.
 */
BOOST_AUTO_TEST_CASE(decode_price_field_4) {
  typedef jb::itch5::price_field<std::uint32_t, 10000> tested;
  using jb::itch5::decoder;

  char buffer[32];
  std::memset(buffer, 0, sizeof(buffer));
  std::memcpy(buffer, "\x00\x12\xD6\x87", 4);
  auto actual = decoder<true, tested>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual.as_integer(), 1234567);
  BOOST_CHECK_CLOSE(actual.as_double(), 123.4567, 0.0001);

  actual = decoder<false, tested>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual.as_integer(), 1234567);
  BOOST_CHECK_CLOSE(actual.as_double(), 123.4567, 0.0001);

  std::memset(buffer, 0, sizeof(buffer));
  std::memcpy(buffer, "\x0D\xFB\x38\xD2", 4);
  actual = decoder<true, tested>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual.as_integer(), 234567890);
  BOOST_CHECK_CLOSE(actual.as_double(), 23456.7890, 0.0001);

  actual = decoder<false, tested>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual.as_integer(), 234567890);
  BOOST_CHECK_CLOSE(actual.as_double(), 23456.7890, 0.0001);
}

/**
 * @test Verify that jb::itch5::decoder for jb::itch5::price_field
 * works as expected.
 */
BOOST_AUTO_TEST_CASE(decode_price_field_8) {
  typedef jb::itch5::price_field<std::uint64_t, 100000000> tested;
  using jb::itch5::decoder;

  char buffer[32];
  std::memset(buffer, 0, sizeof(buffer));
  std::memcpy(buffer, "\x00\x04\x62\xD5\x37\xE7\xEF\x4E", 8);
  auto actual = decoder<true, tested>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual.as_integer(), 1234567812345678ULL);
  BOOST_CHECK_CLOSE(actual.as_double(), 12345678.12345678, 0.0001);

  actual = decoder<false, tested>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual.as_integer(), 1234567812345678ULL);
  BOOST_CHECK_CLOSE(actual.as_double(), 12345678.12345678, 0.0001);
}

/**
 * @test Verify that jb::itch5::price_field iostream operator
 * works as expected.
 */
BOOST_AUTO_TEST_CASE(stream_price_field_4) {
  typedef jb::itch5::price_field<std::uint32_t, 10000> tested;
  using jb::itch5::decoder;
  char buffer[32];

  std::memset(buffer, 0, sizeof(buffer));
  std::memcpy(buffer, "\x00\xBC\x4B\x9B", 4);
  auto actual = decoder<true, tested>::r(16, buffer, 0);

  std::ostringstream os;
  os << actual;
  BOOST_CHECK_EQUAL(os.str(), "1234.0123");
}

/**
 * @test Verify that jb::itch5::price_field iostream operator
 * works as expected.
 */
BOOST_AUTO_TEST_CASE(stream_price_field_8) {
  typedef jb::itch5::price_field<std::uint64_t, 100000000> tested;
  using jb::itch5::decoder;
  char buffer[32];

  std::memset(buffer, 0, sizeof(buffer));
  std::memcpy(buffer, "\x00\x2B\xDC\x54\x5D\x58\xA3\xE0", 8);
  auto actual = decoder<true, tested>::r(16, buffer, 0);

  std::ostringstream os;
  os << actual;
  BOOST_CHECK_EQUAL(os.str(), "123456789.00012000");
}
