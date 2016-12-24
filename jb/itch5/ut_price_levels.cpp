#include "jb/itch5/price_levels.hpp"

#include <boost/test/unit_test.hpp>

/**
 * Helper functions to test jb::itch5::price_levels()
 */
namespace {
template <typename price_field_t>
void check_price_levels() {
  auto d = price_field_t::denom;
  price_field_t p0_9995 = price_field_t(int(0.9995 * d));
  price_field_t p0_9999 = price_field_t(int(0.9999 * d));
  price_field_t p1_00 = price_field_t(int(1.00 * d));
  price_field_t p10_00 = price_field_t(int(10.00 * d));
  price_field_t p10_01 = price_field_t(int(10.01 * d));
  price_field_t p11_01 = price_field_t(int(11.01 * d));

  BOOST_CHECK_EQUAL(jb::itch5::price_levels(p1_00, p1_00), 0);
  BOOST_CHECK_EQUAL(jb::itch5::price_levels(p10_00, p10_01), 1);
  BOOST_CHECK_EQUAL(jb::itch5::price_levels(p10_01, p11_01), 100);
  BOOST_CHECK_EQUAL(jb::itch5::price_levels(p0_9995, p1_00), 5);
  BOOST_CHECK_EQUAL(jb::itch5::price_levels(p0_9995, p0_9999), 4);
  BOOST_CHECK_EQUAL(jb::itch5::price_levels(p0_9995, p11_01), 1006);

  BOOST_CHECK_THROW(jb::itch5::price_levels(p10_01, p10_00), std::range_error);
}

template <typename price_field_t>
void check_level_to_price() {
  auto d = price_field_t::denom;
  price_field_t p0_5000 = price_field_t(int(0.5000 * d));
  price_field_t p1_00 = price_field_t(int(1.00 * d));
  price_field_t p10_00 = price_field_t(int(10.00 * d));

  BOOST_CHECK_EQUAL(
      jb::itch5::level_to_price<price_field_t>(int(0.5000 * d)), p0_5000);
  BOOST_CHECK_EQUAL(
      jb::itch5::level_to_price<price_field_t>(int(1.00 * d)), p1_00);
  BOOST_CHECK_EQUAL(
      jb::itch5::level_to_price<price_field_t>(int(1.00 * d + 9 * d / 100)),
      p10_00);

  BOOST_CHECK_THROW(
      jb::itch5::level_to_price<price_field_t>(int(-1.00 * d)),
      std::range_error);
  price_field_t const max_price =
      jb::itch5::max_price_field_value<price_field_t>();
  std::size_t const max_level =
      jb::itch5::price_levels(price_field_t(0), max_price);
  BOOST_CHECK_THROW(
      jb::itch5::level_to_price<price_field_t>(max_level + 1),
      std::range_error);
}

} // anonymous namespace

/**
 * Verify that jb::itch5::price_levels() works correctly for price4_t
 */
BOOST_AUTO_TEST_CASE(price_levels_4) {
  check_price_levels<jb::itch5::price4_t>();
}

/**
 * Verify that jb::itch5::price_levels() works correctly for price8_t
 */
BOOST_AUTO_TEST_CASE(price_levels_8) {
  check_price_levels<jb::itch5::price8_t>();
}

/**
 * Verify that jb::itch5::level_to_price() works correctly for price4_t
 */
BOOST_AUTO_TEST_CASE(level_to_price_4) {
  check_level_to_price<jb::itch5::price4_t>();
}

/**
 * Verify that jb::itch5::level_to_price() works correctly for price8_t
 */
BOOST_AUTO_TEST_CASE(level_to_price_8) {
  check_level_to_price<jb::itch5::price8_t>();
}
