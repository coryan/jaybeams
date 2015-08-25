#include <jb/itch5/p2ceil.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that p2ceil works correctly for 64-bit numbers.
 */
BOOST_AUTO_TEST_CASE(p2ceil_simple_64) {
  BOOST_CHECK_EQUAL(jb::itch5::p2ceil(std::uint64_t(3)), 4);
  BOOST_CHECK_EQUAL(jb::itch5::p2ceil(std::uint64_t(4)), 8);
  BOOST_CHECK_EQUAL(jb::itch5::p2ceil(std::uint64_t(8)), 16);
  BOOST_CHECK_EQUAL(jb::itch5::p2ceil(std::uint64_t(17)), 32);
  BOOST_CHECK_EQUAL(jb::itch5::p2ceil(std::uint64_t(2317)), 4096);
}

/**
 * @test Verify that p2ceil is known at compile-time.
 */
BOOST_AUTO_TEST_CASE(p2ceil_compile_time) {
  constexpr std::uint64_t actual = jb::itch5::p2ceil(std::uint64_t(17));
  static_assert(actual == 32, "Error in p2ceil static assert");
  BOOST_CHECK_EQUAL(actual, 32);
}

/**
 * @test Verify that p2ceil works correctly for 32-bit numbers.
 */
BOOST_AUTO_TEST_CASE(p2ceil_simple_32) {
  BOOST_CHECK_EQUAL(jb::itch5::p2ceil(std::uint32_t(3)), 4);
  BOOST_CHECK_EQUAL(jb::itch5::p2ceil(std::uint32_t(4)), 8);
  BOOST_CHECK_EQUAL(jb::itch5::p2ceil(std::uint32_t(8)), 16);
  BOOST_CHECK_EQUAL(jb::itch5::p2ceil(std::uint32_t(17)), 32);
  BOOST_CHECK_EQUAL(jb::itch5::p2ceil(std::uint32_t(2317)), 4096);
}

/**
 * @test Verify that p2ceil works correctly for 16-bit numbers.
 */
BOOST_AUTO_TEST_CASE(p2ceil_simple_16) {
  BOOST_CHECK_EQUAL(jb::itch5::p2ceil(std::uint16_t(3)), 4);
  BOOST_CHECK_EQUAL(jb::itch5::p2ceil(std::uint16_t(4)), 8);
  BOOST_CHECK_EQUAL(jb::itch5::p2ceil(std::uint16_t(8)), 16);
  BOOST_CHECK_EQUAL(jb::itch5::p2ceil(std::uint16_t(17)), 32);
  BOOST_CHECK_EQUAL(jb::itch5::p2ceil(std::uint16_t(2317)), 4096);
}

/**
 * @test Verify that p2ceil works correctly for 8-bit numbers.
 */
BOOST_AUTO_TEST_CASE(p2ceil_simple_8) {
  BOOST_CHECK_EQUAL(jb::itch5::p2ceil(std::uint8_t(3)), 4);
  BOOST_CHECK_EQUAL(jb::itch5::p2ceil(std::uint8_t(4)), 8);
  BOOST_CHECK_EQUAL(jb::itch5::p2ceil(std::uint8_t(8)), 16);
  BOOST_CHECK_EQUAL(jb::itch5::p2ceil(std::uint8_t(17)), 32);
  BOOST_CHECK_EQUAL(jb::itch5::p2ceil(std::uint8_t(125)), 128);
}
