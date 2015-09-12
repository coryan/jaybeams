#include <jb/itch5/compute_inside.hpp>

#include <skye/mock_function.hpp>
#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::itch5::order_book works as expected.
 */
BOOST_AUTO_TEST_CASE(compute_inside_trivial) {
  using namespace jb::itch5;

  skye::mock_function<void(
      compute_inside::time_point, stock_t,
      half_quote const&, half_quote const&)> callback;

  jb::itch5::compute_inside tested(callback);

  callback.check_called().never();

  auto now = tested.now();
  tested.handle_message(
      now, 0, 0, // don't care
      stock_directory_message{
        {stock_directory_message::message_type, 0, 0,
          {std::chrono::nanoseconds(0)}
        }, // message_header
        stock_t("HSART"), market_category_t(u'Q'),
        financial_status_indicator_t(u'N'), 100, roundlots_only_t('N'),
        issue_classification_t(u'C'), issue_subtype_t("C"),
        authenticity_t(u'P'), short_sale_threshold_indicator_t(u' '),
        ipo_flag_t(u'N'), luld_reference_price_tier_t(u' '),
        etp_flag_t(u'N'), 0, inverse_indicator_t(u'N') });
  callback.check_called().never();
}
