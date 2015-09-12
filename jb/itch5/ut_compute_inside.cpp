#include <jb/itch5/compute_inside.hpp>

#include <skye/mock_function.hpp>
#include <boost/test/unit_test.hpp>

using namespace jb::itch5;

namespace {

/// Create a stock_directory_message for testing
stock_directory_message create_stock_directory(char const* symbol) {
  return stock_directory_message{
    {stock_directory_message::message_type, 0, 0,
      {std::chrono::nanoseconds(0)}
    }, // message_header
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
BOOST_AUTO_TEST_CASE(compute_inside_trivial) {
  // We are going to use a mock function to handle the callback
  // because it is easy to test what values they got ...
  skye::mock_function<void(
      compute_inside::time_point, stock_t,
      half_quote const&, half_quote const&)> callback;

  // ... create a handler ...
  jb::itch5::compute_inside tested(callback);

  // ... we do not expect any callbacks ...
  callback.check_called().never();

  // ... send a couple of stock directory messages, do not much care
  // about their contents other than the symbol ...
  auto now = tested.now();
  tested.handle_message(
      now, 0, 0, create_stock_directory("HSART"));
  tested.handle_message(
      now, 0, 0, create_stock_directory("FOO"));
  tested.handle_message(
      now, 0, 0, create_stock_directory("BAR"));
  tested.handle_message(
      now, 0, 0, create_stock_directory("HSART"));
  callback.check_called().never();
}
