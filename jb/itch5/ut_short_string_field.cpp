#include <jb/itch5/short_string_field.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::itch5::decoder for jb::itch5::short_string_field
 * works as expected.
 */
BOOST_AUTO_TEST_CASE(decode_short_string_field) {
  typedef jb::itch5::short_string_field<4> tested;
  using jb::itch5::decoder;
  char buffer[32];

  {
    std::memset(buffer, 0, sizeof(buffer));
    std::memcpy(buffer, "AB  ", 4);
    auto actual = decoder<true, tested>::r(16, buffer, 0);
    BOOST_CHECK_EQUAL(actual.c_str(), "AB");

    actual = decoder<false, tested>::r(16, buffer, 0);
    BOOST_CHECK_EQUAL(actual.c_str(), "AB");

    BOOST_CHECK_NO_THROW((decoder<true, tested>::r(16, buffer, 2)));
    BOOST_CHECK_THROW(
        (decoder<true, tested>::r(16, buffer, 13)), std::runtime_error);
    BOOST_CHECK_NO_THROW((decoder<false, tested>::r(16, buffer, 13)));
  }

  {
    std::memset(buffer, 0, sizeof(buffer));
    std::memcpy(buffer, "ABCD", 4);
    auto actual = decoder<true, tested>::r(16, buffer, 0);
    BOOST_CHECK_EQUAL(actual.c_str(), "ABCD");

    actual = decoder<false, tested>::r(16, buffer, 0);
    BOOST_CHECK_EQUAL(actual.c_str(), "ABCD");
  }
}

/**
 * @test Verify that value validators in jb::itch5::decoder works work
 * as expected.
 */
BOOST_AUTO_TEST_CASE(validate_short_string_field) {
  struct simple_validator {
    bool operator()(char const* rhs) const {
      return std::string(rhs) == "AA" or std::string(rhs) == "ABCD";
    }
  };

  using namespace jb::itch5;
  typedef short_string_field<4, simple_validator> tested;

  char buffer[32];
  std::memset(buffer, 0, sizeof(buffer));
  std::memcpy(buffer, "ABCD", 4);
  auto actual = decoder<true, tested>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual.c_str(), "ABCD");

  std::memset(buffer, 0, sizeof(buffer));
  std::memcpy(buffer, "ABC", 4);
  BOOST_CHECK_THROW(
      (decoder<true, tested>::r(16, buffer, 0)), std::runtime_error);
  BOOST_CHECK_NO_THROW((decoder<false, tested>::r(16, buffer, 0)));
}

/**
 * @test Verify that jb::itch5::short_string_field iostream operator
 * works as expected.
 */
BOOST_AUTO_TEST_CASE(stream_short_string_field) {
  typedef jb::itch5::short_string_field<4> tested;
  using jb::itch5::decoder;
  char buffer[32];

  {
    std::memset(buffer, 0, sizeof(buffer));
    std::memcpy(buffer, "AB  ", 4);
    auto actual = decoder<true, tested>::r(16, buffer, 0);

    std::ostringstream os;
    os << actual;
    BOOST_CHECK_EQUAL(os.str(), "AB");
  }

  {
    std::memset(buffer, 0, sizeof(buffer));
    std::memcpy(buffer, "ABCD", 4);
    auto actual = decoder<true, tested>::r(16, buffer, 0);

    std::ostringstream os;
    os << actual;
    BOOST_CHECK_EQUAL(os.str(), "ABCD");
  }
}
