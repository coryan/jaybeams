#include "jb/itch5/compute_book.hpp"
#include <jb/as_hhmmss.hpp>

#include <skye/mock_function.hpp>
#include <boost/test/unit_test.hpp>
#include <thread>

using namespace jb::itch5;

/**
 * Helper functions and constants to test jb::itch5::compute_book.
 */
namespace {
buy_sell_indicator_t const BUY(u'B');
buy_sell_indicator_t const SELL(u'S');
} // anonymous namespace

/**
 * @test Verify that jb::itch5::compute_book::book_update operators
 * work as expected.
 */
BOOST_AUTO_TEST_CASE(compute_book_book_update_operators) {
  auto const ts0 = compute_book::clock_type::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  auto const ts1 = compute_book::clock_type::now();
  BOOST_CHECK_EQUAL(
      compute_book::book_update({ts0, stock_t("A"), BUY, price4_t(1000), 100}),
      compute_book::book_update({ts0, stock_t("A"), BUY, price4_t(1000), 100}));

  BOOST_CHECK_NE(
      compute_book::book_update({ts0, stock_t("A"), BUY, price4_t(1000), 100}),
      compute_book::book_update({ts1, stock_t("A"), BUY, price4_t(1000), 100}));
  BOOST_CHECK_NE(
      compute_book::book_update({ts0, stock_t("A"), BUY, price4_t(1000), 100}),
      compute_book::book_update({ts0, stock_t("B"), BUY, price4_t(1000), 100}));
  BOOST_CHECK_NE(
      compute_book::book_update({ts0, stock_t("A"), BUY, price4_t(1000), 10}),
      compute_book::book_update({ts0, stock_t("A"), SELL, price4_t(1000), 10}));
  BOOST_CHECK_NE(
      compute_book::book_update({ts0, stock_t("A"), BUY, price4_t(1000), 100}),
      compute_book::book_update({ts0, stock_t("A"), BUY, price4_t(1001), 100}));
  BOOST_CHECK_NE(
      compute_book::book_update({ts0, stock_t("A"), BUY, price4_t(1000), 100}),
      compute_book::book_update({ts0, stock_t("B"), BUY, price4_t(1000), 200}));
}
