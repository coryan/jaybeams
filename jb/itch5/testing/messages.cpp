#include "jb/itch5/testing/messages.hpp"

namespace jb {
namespace itch5 {
namespace testing {

stock_directory_message create_stock_directory(char const* symbol) {
  return stock_directory_message{
      {stock_directory_message::message_type, 0, 0,
       jb::itch5::timestamp{std::chrono::nanoseconds(0)}},
      stock_t(symbol),
      market_category_t(u'Q'),
      financial_status_indicator_t(u'N'),
      100,
      roundlots_only_t('N'),
      issue_classification_t(u'C'),
      issue_subtype_t("C"),
      authenticity_t(u'P'),
      short_sale_threshold_indicator_t(u' '),
      ipo_flag_t(u'N'),
      luld_reference_price_tier_t(u' '),
      etp_flag_t(u'N'),
      0,
      inverse_indicator_t(u'N')};
}

} // namespace testing
} // namespace itch5
} // namespace jb
