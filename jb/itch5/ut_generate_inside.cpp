#include <jb/itch5/generate_inside.hpp>

#include <skye/mock_function.hpp>
#include <boost/test/unit_test.hpp>

#include <sstream>

using namespace jb::itch5;

/**
 * Helper functions and constants to test jb::itch5::generate_inside.
 */
namespace {
buy_sell_indicator_t const BUY(u'B');
buy_sell_indicator_t const SELL(u'S');

jb::itch5::message_header create_header(std::chrono::nanoseconds ns) {
  return jb::itch5::message_header{add_order_message::message_type, 0, 0,
                                   jb::itch5::timestamp{ns}};
}
} // anonymous namespace

/**
 * @test Verify that jb::itch5::generate_inside works as expected.
 */
BOOST_AUTO_TEST_CASE(generate_inside_basic) {
  // ... create the preconditions to run a test, first some place to
  // collect the statistics ...
  jb::offline_feed_statistics stats{jb::offline_feed_statistics::config()};
  // ... then a book with 3 orders on each side, widely spaced 
  jb::itch5::order_book book;
  book.handle_add_order(BUY, price4_t(10 * 10000), 300);
  book.handle_add_order(BUY, price4_t(11 * 10000), 200);
  book.handle_add_order(BUY, price4_t(12 * 10000), 100);
  book.handle_add_order(SELL, price4_t(15 * 10000), 100);
  book.handle_add_order(SELL, price4_t(16 * 10000), 200);
  book.handle_add_order(SELL, price4_t(17 * 10000), 300);

  stock_t stock("HSART");
  std::ostringstream out;

  // ... an update at the BBO produces no output ...
  auto now = jb::itch5::compute_book::clock_type::now();
  jb::itch5::compute_book::clock_type::duration pl(
      std::chrono::nanoseconds(525));
  BOOST_CHECK_EQUAL(
      false,
      generate_inside(
          stats, out, create_header(std::chrono::nanoseconds(0)), book,
          compute_book::book_update{now, stock, BUY, price4_t(10 * 10000), 100},
          pl));
  BOOST_CHECK_EQUAL(std::string(""), out.str());
  BOOST_CHECK_EQUAL(
      false, generate_inside(
                 stats, out, create_header(std::chrono::nanoseconds(0)), book,
                 compute_book::book_update{now, stock, SELL,
                                           price4_t(17 * 10000), 100},
                 pl));
  BOOST_CHECK_EQUAL(std::string(""), out.str());

  // ... an order better than the BBO produces some output ...
  out.str("");
  now = jb::itch5::compute_book::clock_type::now();
  BOOST_CHECK_EQUAL(
      true, generate_inside(
                stats, out, create_header(std::chrono::nanoseconds(0)), book,
                compute_book::book_update{now, stock, BUY,
                                          price4_t(12 * 10000 + 5000), 100},
                pl));
  BOOST_CHECK_EQUAL(
      std::string("0 0 HSART 120000 100 150000 100\n"), out.str());
  out.str("");
  BOOST_CHECK_EQUAL(
      true, generate_inside(
                stats, out, create_header(std::chrono::nanoseconds(0)), book,
                compute_book::book_update{now, stock, SELL,
                                          price4_t(15 * 10000 - 5000), 100},
                pl));
  BOOST_CHECK_EQUAL(
      std::string("0 0 HSART 120000 100 150000 100\n"), out.str());

  // ... an order at the BBO produces some output ...
  out.str("");
  now = jb::itch5::compute_book::clock_type::now();
  BOOST_CHECK_EQUAL(
      true, generate_inside(
                stats, out, create_header(std::chrono::nanoseconds(0)), book,
                compute_book::book_update{now, stock, BUY, price4_t(12 * 10000),
                      100}, pl));
  BOOST_CHECK_EQUAL(
      std::string("0 0 HSART 120000 100 150000 100\n"), out.str());
  out.str("");
  BOOST_CHECK_EQUAL(
      true, generate_inside(
                stats, out, create_header(std::chrono::nanoseconds(0)), book,
                compute_book::book_update{now, stock, SELL,
                                          price4_t(15 * 10000), 100},
                pl));
  BOOST_CHECK_EQUAL(
      std::string("0 0 HSART 120000 100 150000 100\n"), out.str());
}
