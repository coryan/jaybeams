#include <jb/itch5/decoder.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that buffer size validation works as expected.
 */
BOOST_AUTO_TEST_CASE(check_offset_disabled) {
  using jb::itch5::check_offset;
  BOOST_CHECK_NO_THROW(check_offset<false>("test", 4, 2, 1));
  BOOST_CHECK_NO_THROW(check_offset<false>("test", 4, 2, 2));
  BOOST_CHECK_NO_THROW(check_offset<false>("test", 4, 0, 4));
  BOOST_CHECK_NO_THROW(check_offset<false>("test", 4, 0, 8));
  BOOST_CHECK_NO_THROW(check_offset<false>("test", 4, 2, 3));
  BOOST_CHECK_NO_THROW(check_offset<false>("test", 4, 4, 1));
  BOOST_CHECK_NO_THROW(check_offset<false>("test", 4, 4, 0));
}

/**
 * @test Verify that buffer size validation works as expected.
 */
BOOST_AUTO_TEST_CASE(check_offset_enabled) {
  using jb::itch5::check_offset;
  BOOST_CHECK_NO_THROW(check_offset<true>("test", 4, 2, 1));
  BOOST_CHECK_NO_THROW(check_offset<true>("test", 4, 2, 2));
  BOOST_CHECK_NO_THROW(check_offset<true>("test", 4, 0, 4));
  BOOST_CHECK_THROW(check_offset<true>("test", 4, 0, 8), std::runtime_error);
  BOOST_CHECK_THROW(check_offset<true>("test", 4, 2, 3), std::runtime_error);
  BOOST_CHECK_THROW(check_offset<true>("test", 4, 4, 1), std::runtime_error);
  BOOST_CHECK_THROW(check_offset<true>("test", 4, 4, 0), std::runtime_error);
}
