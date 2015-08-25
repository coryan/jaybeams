#ifndef jb_itch5_stock_directory_message_hpp
#define jb_itch5_stock_directory_message_hpp

#include <jb/itch5/char_list_field.hpp>
#include <jb/itch5/message_header.hpp>
#include <jb/itch5/stock_field.hpp>

namespace jb {
namespace itch5 {

/**
 * Represent the 'Market Category' field on a 'Stock Directory' message.
 */
typedef char_list_field<
  u'Q', // NASDAQ Global Select Market
  u'G', // NASDAQ Global Market
  u'S', // NASDAQ Capital Market
  u'N', // New York Stock Exchange
  u'A', // NYSE MKT
  u'P', // NYSE ARCA
  u'Z', // BATS Z Exchange
  u' '  // Not available
  > market_category_t;

/**
 * Represent the 'Financial Status Indicator' field on a 'Stock
 * Directory' message.
 */
typedef char_list_field<
  u'D', // Deficient
  u'E', // Delinquent
  u'Q', // Bankrupt
  u'S', // Suspended
  u'G', // Deficient and Bankrupt
  u'H', // Deficient and Delinquent
  u'J', // Delinquent and Bankrupt
  u'K', // Deficient, Delinquent and Bankrupt
  u'C', // Creations and/or Redemptions Suspended for Exchange Traded
        // Product
  u'N', // Nomal (Default): Issuer is not Deficient, Delinquent or Bankrupt
  u' '  // Not available
  > financial_status_indicator_t;

/**
 * Represent the 'Round Lots Only' field on a 'Stock
 * Directory' message.
 */
typedef char_list_field<u'Y', u'N'> roundlots_only_t;

/**
 * Represent the 'Issue Classification' field on a 'Stock
 * Directory' message.
 */
typedef char_list_field<
  u'A', // American Depositary Share
  u'B', // Bond
  u'C', // Common Stock
  u'F', // Depository Receipt
  u'I', // 144A
  u'L', // Limited Partnership
  u'N', // Notes
  u'O', // Ordinary Share
  u'P', // Preferred Stock
  u'Q', // Other Securities
  u'R', // Right
  u'S', // Shares of Beneficial Interest
  u'T', // Convertible Debenture
  u'U', // Unit
  u'V', // Units/Benif Int
  u'W'  // Warrant
  > issue_classification_t;

/// A functor to validate the 'Issue Sub-Type' field.
struct validate_issue_subtype {
  /// functor operator
  bool operator()(char const* value) const;
};

/**
 * Represent the 'Issue Sub-Type' field on a 'Stock Directory' message.
 */
typedef short_string_field<2,validate_issue_subtype> issue_subtype_t;

/**
 * Represent the 'Authenticity' field on a 'Stock
 * Directory' message.
 */
typedef char_list_field<
  u'P', // Production
  u'T'  // Test
  > authenticity_t;

/**
 * Represent the 'Short Sale Threshold Indicator' field on a 'Stock
 * Directory' message.
 */
typedef char_list_field<u'Y', u'N', u' '> short_sale_threshold_indicator_t;

/**
 * Represent the 'IPO Flag' field on a 'Stock
 * Directory' message.
 */
typedef char_list_field<u'Y', u'N', u' '> ipo_flag_t;

/**
 * Represent the 'LULD Reference Price Tier' field on a 'Stock
 * Directory' message.
 *
 * LULD stands for 'Limit Up, Limit Down', a restriction on pricing to
 * avoid sudden drops or increases in price.
 */
typedef char_list_field<u'1', u'2', u' '> luld_reference_price_tier_t;

/**
 * Represent the 'ETP Flag' field on a 'Stock
 * Directory' message.
 */
typedef char_list_field<u'Y', u'N', u' '> etp_flag_t;

/**
 * Represent the 'Inverse Indicator' field on a 'Stock
 * Directory' message.
 */
typedef char_list_field<u'Y', u'N'> inverse_indicator_t;

/**
 * Represent a 'Stock Directory' message in the ITCH-5.0 protocol.
 */
struct stock_directory_message {
  constexpr static int message_type = u'R';

  message_header header;
  stock_t stock;
  market_category_t market_category;
  financial_status_indicator_t financial_status_indicator;
  int round_lot_size;
  roundlots_only_t roundlots_only;
  issue_classification_t issue_classification;
  issue_subtype_t issue_subtype;
  authenticity_t authenticity;
  short_sale_threshold_indicator_t short_sale_threshold_indicator;
  ipo_flag_t ipo_flag;
  luld_reference_price_tier_t luld_reference_price_tier;
  etp_flag_t etp_flag;
  int etp_leverage_factor;
  inverse_indicator_t inverse_indicator;
};

/// Specialize decoder for a jb::itch5::stock_directory_message
template<bool V>
struct decoder<V,stock_directory_message> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static stock_directory_message r(
      std::size_t size, char const* buf, std::size_t off) {
    stock_directory_message x;
    x.header =
        decoder<V,message_header>              ::r(size, buf, off + 0);
    x.stock =
        decoder<V,stock_t>                     ::r(size, buf, off + 11);
    x.market_category =
        decoder<V,market_category_t>           ::r(size, buf, off + 19);
    x.financial_status_indicator =
        decoder<V,financial_status_indicator_t>::r(size, buf, off + 20);
    x.round_lot_size =
        decoder<V,std::uint32_t>               ::r(size, buf, off + 21);
    x.roundlots_only =
        decoder<V,roundlots_only_t>            ::r(size, buf, off + 25);
    x.issue_classification =
        decoder<V,issue_classification_t>      ::r(size, buf, off + 26);
    x.issue_subtype =
        decoder<V,issue_subtype_t>             ::r(size, buf, off + 27);
    x.authenticity =
        decoder<V,authenticity_t>              ::r(size, buf, off + 29);
    x.short_sale_threshold_indicator =
        decoder<V,short_sale_threshold_indicator_t> ::r(size, buf, off + 30);
    x.ipo_flag =
        decoder<V,ipo_flag_t>                       ::r(size, buf, off + 31);
    x.luld_reference_price_tier =
        decoder<V,luld_reference_price_tier_t>      ::r(size, buf, off + 32);
    x.etp_flag =
        decoder<V,etp_flag_t>                       ::r(size, buf, off + 33);
    x.etp_leverage_factor =
        decoder<V,std::uint32_t>                    ::r(size, buf, off + 34);
    x.inverse_indicator =
        decoder<V,inverse_indicator_t>              ::r(size, buf, off + 38);
    return std::move(x);
  }
};

/// Streaming operator for jb::itch5::stock_directory_message.
std::ostream& operator<<(std::ostream& os, stock_directory_message const& x);

} // namespace itch5
} // namespace jb

#endif /* jb_itch5_stock_directory_message_hpp */
