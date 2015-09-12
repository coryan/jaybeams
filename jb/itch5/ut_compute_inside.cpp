#include <jb/itch5/compute_inside.hpp>
#include <jb/as_hhmmss.hpp>

#include <skye/mock_function.hpp>
#include <boost/test/unit_test.hpp>

using namespace jb::itch5;

namespace std { // oh the horror

bool operator==(
    jb::itch5::half_quote const& lhs, jb::itch5::half_quote const& rhs) {
  return lhs.first == rhs.first and lhs.second == rhs.second;
}

std::ostream& operator<<(std::ostream& os, jb::itch5::half_quote const& x) {
  return os << "{" << x.first << "," << x.second << "}";
}

} // namespace std

namespace {

buy_sell_indicator_t const BUY(u'B');
buy_sell_indicator_t const SELL(u'S');

/// Create a simple timestamp
timestamp create_timestamp() {
  return timestamp{std::chrono::nanoseconds(0)};
}

/// Create a stock_directory_message for testing
stock_directory_message create_stock_directory(char const* symbol) {
  return stock_directory_message{
    {stock_directory_message::message_type, 0, 0, create_timestamp()},
    stock_t(symbol), market_category_t(u'Q'),
    financial_status_indicator_t(u'N'), 100, roundlots_only_t('N'),
    issue_classification_t(u'C'), issue_subtype_t("C"),
    authenticity_t(u'P'), short_sale_threshold_indicator_t(u' '),
    ipo_flag_t(u'N'), luld_reference_price_tier_t(u' '),
    etp_flag_t(u'N'), 0, inverse_indicator_t(u'N') };
}

} // anonymous namespace

/**
 * @test Verify that jb::itch5::order_book works as expected.
 */
BOOST_AUTO_TEST_CASE(compute_inside_simple) {
  // We are going to use a mock function to handle the callback
  // because it is easy to test what values they got ...
  skye::mock_function<void(
      compute_inside::time_point, stock_t,
      half_quote const&, half_quote const&)> callback;

  // ... create a handler ...
  auto cb = [&callback](compute_inside::time_point a, stock_t b,
                        half_quote const& c, half_quote const& d) {
    callback(a, b, c, d);
  };
  compute_inside tested(cb);

  // ... we do not expect any callbacks ...
  callback.check_called().never();

  // ... send a couple of stock directory messages, do not much care
  // about their contents other than the symbol ...
  compute_inside::time_point now = tested.now();
  long msgcnt = 0;
  tested.handle_message(
      now, ++msgcnt, 0, create_stock_directory("HSART"));
  tested.handle_message(
      now, ++msgcnt, 0, create_stock_directory("FOO"));
  tested.handle_message(
      now, ++msgcnt, 0, create_stock_directory("BAR"));
  // ... duplicates should not create a problem ...
  tested.handle_message(
      now, ++msgcnt, 0, create_stock_directory("HSART"));
  callback.check_called().never();

  // ... handle a new order ...
  tested.handle_message(
      now, ++msgcnt, 0, add_order_message{
        {add_order_message::message_type, 0, 0, create_timestamp()},
         1, BUY, 100, stock_t("HSART"), price4_t(100000)} );
  callback.check_called().once().with(
      compute_inside::time_point(now), stock_t("HSART"),
      half_quote(price4_t(100000), 100), order_book::empty_offer());

  tested.handle_message(
      now, ++msgcnt, 0, add_order_message{
        {add_order_message::message_type, 0, 0, create_timestamp()},
         1, SELL, 100, stock_t("HSART"), price4_t(100100)} );
  callback.check_called().once().with(
      compute_inside::time_point(now), stock_t("HSART"),
      half_quote(price4_t(100000), 100), half_quote(price4_t(100100), 100));
}

