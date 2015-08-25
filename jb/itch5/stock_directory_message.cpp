#include <jb/itch5/stock_directory_message.hpp>

#include <iostream>
#include <mutex>
#include <unordered_map>
#include <string>

namespace {
typedef std::unordered_map<std::string,std::string> valid_subtype_t;

valid_subtype_t valid_subtypes;
std::once_flag valid_subtypes_initialized;

void initialize_valid_subtypes();
} // anonymous namespace

constexpr int jb::itch5::stock_directory_message::message_type;

bool jb::itch5::validate_issue_subtype::operator()(char const* value) const {
  std::call_once(valid_subtypes_initialized, initialize_valid_subtypes);

  return valid_subtypes.find(value) != valid_subtypes.end();
}

std::ostream& jb::itch5::operator<<(
    std::ostream& os, stock_directory_message const& x) {
  return os << x.header
            << ",stock=" << x.stock
            << ",market_category=" << x.market_category
            << ",financial_status_indicator=" << x.financial_status_indicator
            << ",round_lot_size=" << x.round_lot_size
            << ",roundlots_only=" << x.roundlots_only
            << ",issue_classification=" << x.issue_classification
            << ",issue_subtype=" << x.issue_subtype
            << ",authenticity=" << x.authenticity
            << ",short_sale_threshold_indicator="
            << x.short_sale_threshold_indicator
            << ",ipo_flag=" << x.ipo_flag
            << ",luld_reference_price_tier=" << x.luld_reference_price_tier
            << ",etp_flag=" << x.etp_flag
            << ",etp_leverage_factor=" << x.etp_leverage_factor
            << ",inverse_indicator=" << x.inverse_indicator
      ;
}

namespace {
void initialize_valid_subtypes() {
  valid_subtypes.emplace(
      "A", "Preferred Trust Securities");
  valid_subtypes.emplace(
      "AI", "Alpha Index ETNs");
  valid_subtypes.emplace(
      "B", "Index Based Derivative");
  valid_subtypes.emplace(
      "C", "Common Shares");
  valid_subtypes.emplace(
      "CB", "Commodity Based Trust Shares");
  valid_subtypes.emplace(
      "CF", "Commodity Futures Trust Shares");
  valid_subtypes.emplace(
      "CL", "Commodity-Linked Securities");
  valid_subtypes.emplace(
      "CM", "Commodity Index Trust Shares");
  valid_subtypes.emplace(
      "CO", "Collateralized Mortgage Obligation");
  valid_subtypes.emplace(
      "CT", "Currency Trust Shares");
  valid_subtypes.emplace(
      "CU", "Commodity-Currency-Linked Securities");
  valid_subtypes.emplace(
      "CW", "Currency Warrants");
  valid_subtypes.emplace(
      "D", "Global Depositary Shares");
  valid_subtypes.emplace(
      "E", "ETF-Portfolio Depositary Receipt");
  valid_subtypes.emplace(
      "EG", "Equity Gold Shares");
  valid_subtypes.emplace(
      "EI", "ETN-Equity Index-Linked Securities");
  valid_subtypes.emplace(
      "EM", "Exchange Traded Managed Funds*");
  valid_subtypes.emplace(
      "EN", "Exchange Traded Notes");
  valid_subtypes.emplace(
      "EU", "Equity Units");
  valid_subtypes.emplace(
      "F", "HOLDRS");
  valid_subtypes.emplace(
      "FI", "ETN-Fixed Income-Linked Securities");
  valid_subtypes.emplace(
      "FL", "ETN-Futures-Linked Securities");
  valid_subtypes.emplace(
      "G", "Global Shares");
  valid_subtypes.emplace(
      "I", "ETF-Index Fund Shares");
  valid_subtypes.emplace(
      "IR", "Interest Rate");
  valid_subtypes.emplace(
      "IW", "Index Warrant");
  valid_subtypes.emplace(
      "IX", "Index-Linked Exchangeable Notes");
  valid_subtypes.emplace(
      "J", "Corporate Backed Trust Security");
  valid_subtypes.emplace(
      "L", "Contingent Litigation Right");
  valid_subtypes.emplace(
      "LL", "Identifies securities of companies that are set up "
      "as a Limited Liability Company (LLC)");
  valid_subtypes.emplace(
      "M", "Equity-Based Derivative");
  valid_subtypes.emplace(
      "MF", "Managed Fund Shares");
  valid_subtypes.emplace(
      "ML", "ETN-Multi-Factor Index-Linked Securities");
  valid_subtypes.emplace(
      "MT", "Managed Trust Securities");
  valid_subtypes.emplace(
      "N", "NY Registry Shares");
  valid_subtypes.emplace(
      "O", "Open Ended Mutual Fund");
  valid_subtypes.emplace(
      "P", "Privately Held Security");
  valid_subtypes.emplace(
      "PP", "Poison Pill");
  valid_subtypes.emplace(
      "PU", "Partnership Units");
  valid_subtypes.emplace(
      "Q", "Closed-End Funds");
  valid_subtypes.emplace(
      "R", "Reg-S");
  valid_subtypes.emplace(
      "RC", "Commodity-Redeemable Commodity-Linked Securities");
  valid_subtypes.emplace(
      "RF", "ETN-Redeemable Futures-Linked Securities");
  valid_subtypes.emplace(
      "RT", "REIT");
  valid_subtypes.emplace(
      "RU", "Commodity-Redeemable Currency-Linked Securities");
  valid_subtypes.emplace(
      "S", "SEED");
  valid_subtypes.emplace(
      "SC", "Spot Rate Closing");
  valid_subtypes.emplace(
      "SI", "Spot Rate Intraday");
  valid_subtypes.emplace(
      "T", "Tracking Stock");
  valid_subtypes.emplace(
      "TC", "Trust Certificates");
  valid_subtypes.emplace(
      "TU", "Trust Units");
  valid_subtypes.emplace(
      "U", "Portal");
  valid_subtypes.emplace(
      "V", "Contingent Value Right");
  valid_subtypes.emplace(
      "W", "Trust Issued Receipts");
  valid_subtypes.emplace(
      "WC", "World Currency Option");
  valid_subtypes.emplace(
      "X", "Trust");
  valid_subtypes.emplace(
      "Y", "Other");
  valid_subtypes.emplace(
      "Z", "Not Applicable");
}

} // anonymous namespace
