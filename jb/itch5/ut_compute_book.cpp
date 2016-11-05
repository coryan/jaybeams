#include <jb/itch5/compute_book.hpp>
#include <jb/itch5/testing/messages.hpp>
#include <jb/as_hhmmss.hpp>

#include <skye/mock_function.hpp>
#include <boost/test/unit_test.hpp>
#include <algorithm>
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
 * @test Verify that jb::itch5::compute_book::handle_message works as
 * expected for stock_directory_message.
 */
BOOST_AUTO_TEST_CASE(compute_book_stock_directory_message) {
  skye::mock_function<void()> callback;
  auto cb = [&callback](
      message_header const&, compute_book::book_update const& update,
      order_book const& updated_book) { callback(); };

  compute_book tested(cb);

  using namespace jb::itch5::testing;

  compute_book::time_point now = tested.now();
  long msgcnt = 0;
  tested.handle_message(now, ++msgcnt, 0, create_stock_directory("HSART"));
  tested.handle_message(now, ++msgcnt, 0, create_stock_directory("FOO"));
  tested.handle_message(now, ++msgcnt, 0, create_stock_directory("B"));

  std::vector<stock_t> expected{stock_t("HSART"), stock_t("FOO"), stock_t("B")};
  auto actual = tested.symbols();
  // ... the symbols are not guaranteed to return in any particular
  // order, so sort them to make testing easier ...
  std::sort(expected.begin(), expected.end());
  std::sort(actual.begin(), actual.end());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      expected.begin(), expected.end(), actual.begin(), actual.end());

  // ... a repeated symbol should have no effect ...
  tested.handle_message(now, ++msgcnt, 0, create_stock_directory("B"));
  actual = tested.symbols();
  std::sort(expected.begin(), expected.end());
  std::sort(actual.begin(), actual.end());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      expected.begin(), expected.end(), actual.begin(), actual.end());
}


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
