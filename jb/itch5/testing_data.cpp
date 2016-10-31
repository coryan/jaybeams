#include "jb/itch5/testing_data.hpp"

#include <jb/itch5/timestamp.hpp>
#include <jb/itch5/protocol_constants.hpp>

#include <sstream>
#include <stdexcept>

namespace jb {
namespace itch5 {
namespace testing {

/// A convenient sequence of bytes to test messages.
#define JB_ITCH5_TEST_HEADER                                                   \
  "\x00\x00"                 /* Stock Locate    (0) */                         \
  "\x00\x01"                 /* Tracking Number (1) */                         \
  "\x25\xCA\x5F\xF4\x23\x15" /* Timestamp       (11:32:30.123456789) */

std::chrono::nanoseconds expected_ts() {
  using namespace std::chrono;
  return duration_cast<nanoseconds>(hours(11) + minutes(32) + seconds(31) +
                                    nanoseconds(123456789L));
}

std::pair<char const*, std::size_t> message_header() {
  static char const buf[] = u8" " // Message Type (space is invalid)
      JB_ITCH5_TEST_HEADER        // Common header body
      ;
  std::size_t const bufsize = sizeof(buf) - 1;
  return std::make_pair(buf, bufsize);
}

std::pair<char const*, std::size_t> add_order() {
  static char const buf[] = u8"A" // Message Type
      JB_ITCH5_TEST_HEADER        // Common test header
                            "\x00\x00\x00\x00"
                            "\x00\x00\x00\x2A" // Order Reference Number (42)
                            "B"                // Buy/Sell Indicator
                            "\x00\x00\x00\x64" // Shares (100)
                            "HSART   "         // Stock
                            "\x00\x12\xC6\xA4" // Price (123.0500)
      ;
  std::size_t const bufsize = sizeof(buf) - 1;
  return std::make_pair(buf, bufsize);
}

std::pair<char const*, std::size_t> add_order_mpid() {
  static char const buf[] = u8"F" // Message Type
      JB_ITCH5_TEST_HEADER        // Common test header
                            "\x00\x00\x00\x00"
                            "\x00\x00\x00\x2A" // Order Reference Number (42)
                            "B"                // Buy/Sell Indicator
                            "\x00\x00\x00\x64" // Shares (100)
                            "HSART   "         // Stock
                            "\x00\x12\xC6\xA4" // Price (123.0500)
                            "LOOF"             // MPID
      ;
  std::size_t const bufsize = sizeof(buf) - 1;
  return std::make_pair(buf, bufsize);
}

std::pair<char const*, std::size_t> broken_trade() {
  static char const buf[] = u8"B" // Message Type
      JB_ITCH5_TEST_HEADER        // Common test header
                            "\x00\x00\x00\x00"
                            "\x00\x23\xB6\xF8" // Match Number (2340600)
      ;
  std::size_t const bufsize = sizeof(buf) - 1;
  return std::make_pair(buf, bufsize);
}

std::pair<char const*, std::size_t> cross_trade() {
  static char const buf[] = u8"Q" // Message Type
      JB_ITCH5_TEST_HEADER        // Common test header
                            "\x00\x00\x00\x00"
                            "\x00\x06\x79\x08" // Shares (424200)
                            "HSART   "         // Stock
                            "\x00\x12\xC6\xA4" // Cross Price (123.0500)
                            "\x00\x00\x00\x00"
                            "\x00\x23\xB6\xF8" // Match Number (2340600)
                            "H"                // Cross Type (H)
      ;
  std::size_t const bufsize = sizeof(buf) - 1;
  return std::make_pair(buf, bufsize);
}

std::pair<char const*, std::size_t> ipo_quoting_period_update() {
  static char const buf[] =
      u8"K"                // Message Type
      JB_ITCH5_TEST_HEADER // Common test header
      "HSART   "           // Stock
      "\x00\x00\xC0\xFD"   // IPO Quotation Release Time (13:43:25)
      "A"                  // IPO Quotation Release Qualifier
      "\x00\x12\xC6\xA4"   // IPO Price (123.0500)
      ;
  std::size_t const bufsize = sizeof(buf) - 1;
  return std::make_pair(buf, bufsize);
}

std::pair<char const*, std::size_t> market_participant_position() {
  static char const buf[] = u8"L"      // Message Type
      JB_ITCH5_TEST_HEADER             // Common test header
                            "LOOF"     // MPID
                            "HSART   " // Stock
                            "N"        // Primary Market Maker
                            "N"        // Market Maker Mode
                            "A"        // Market Participant State
      ;
  std::size_t const bufsize = sizeof(buf) - 1;
  return std::make_pair(buf, bufsize);
}

std::pair<char const*, std::size_t> mwcb_breach() {
  static char const buf[] = u8"W" // Message Type
      JB_ITCH5_TEST_HEADER        // Common test header
                            "2"   // Breached Level
      ;
  std::size_t const bufsize = sizeof(buf) - 1;
  return std::make_pair(buf, bufsize);
}

std::pair<char const*, std::size_t> mwcb_decline_level() {
  static char const buf[] =
      u8"V"                              // Message Type
      JB_ITCH5_TEST_HEADER               // Common test header
      "\x00\x00\x00\x74\x6A\x61\xCA\x40" // Level 1 (5000.01)
      "\x00\x00\x00\x5D\x21\xEB\x30\x60" // Level 2 (4000.0102)
      "\x00\x00\x00\x45\xD9\x74\x49\x8C" // Level 3 (3000.010203)
      ;
  std::size_t const bufsize = sizeof(buf) - 1;
  return std::make_pair(buf, bufsize);
}

std::pair<char const*, std::size_t> net_order_imbalance_indicator() {
  static char const buf[] =
      u8"I"                // Message Type
      JB_ITCH5_TEST_HEADER // Common test header
      "\x00\x00\x00\x00"
      "\x02\x80\xDE\x80" // Paired Shares (42000000)
      "\x00\x00\x00\x00"
      "\x00\x06\x79\x08" // Imbalance Shares (424200)
      "B"                // Imbalance Direction (H)
      "HSART   "         // Stock
      "\x00\x23\xB6\xF8" // Far Price (234.0600)
      "\x00\x12\xC6\xA4" // Near Price (123.0500)
      "\x00\x0D\x94\xF4" // Current Reference Price (89.0100)
      "O"                // Cross Type (O)
      "A"                // Price Variation Indicator
      ;
  std::size_t const bufsize = sizeof(buf) - 1;
  return std::make_pair(buf, bufsize);
}

std::pair<char const*, std::size_t> order_cancel() {
  static char const buf[] = u8"X" // Message Type
      JB_ITCH5_TEST_HEADER        // Common test header
                            "\x00\x00\x00\x00"
                            "\x00\x00\x00\x2A" // Order Reference Number (42)
                            "\x00\x00\x01\x2C" // Canceled Shares (300)
      ;
  std::size_t const bufsize = sizeof(buf) - 1;
  return std::make_pair(buf, bufsize);
}

std::pair<char const*, std::size_t> order_delete() {
  static char const buf[] = u8"D" // Message Type
      JB_ITCH5_TEST_HEADER        // Common test header
                            "\x00\x00\x00\x00"
                            "\x00\x00\x00\x2A" // Order Reference Number (42)
      ;
  std::size_t const bufsize = sizeof(buf) - 1;
  return std::make_pair(buf, bufsize);
}

std::pair<char const*, std::size_t> order_executed() {
  static char const buf[] = u8"E" // Message Type
      JB_ITCH5_TEST_HEADER        // Common test header
                            "\x00\x00\x00\x00"
                            "\x00\x00\x00\x2A" // Order Reference Number (42)
                            "\x00\x00\x01\x2C" // Executed Shares (300)
                            "\x00\x00\x00\x00"
                            "\x00\x00\x01\x3D" // Match Number (317)
      ;
  std::size_t const bufsize = sizeof(buf) - 1;
  return std::make_pair(buf, bufsize);
}

std::pair<char const*, std::size_t> order_executed_price() {
  static char const buf[] = u8"C" // Message Type
      JB_ITCH5_TEST_HEADER        // Common test header
                            "\x00\x00\x00\x00"
                            "\x00\x00\x00\x2A" // Order Reference Number (42)
                            "\x00\x00\x01\x2C" // Executed Shares (300)
                            "\x00\x00\x00\x00"
                            "\x00\x00\x01\x3D" // Match Number (317)
                            "Y"                // Printable (Y)
                            "\x00\x12\xC6\xA4" // Execution Price (123.0500)
      ;
  std::size_t const bufsize = sizeof(buf) - 1;
  return std::make_pair(buf, bufsize);
}

std::pair<char const*, std::size_t> order_replace() {
  static char const buf[] =
      u8"U"                // Message Type
      JB_ITCH5_TEST_HEADER // Common test header
      "\x00\x00\x00\x00"
      "\x00\x00\x00\x2A" // Original Order Reference Number (42)
      "\x00\x00\x00\x00"
      "\x00\x00\x10\x92" // New Order Reference Number (4242)
      "\x00\x00\x00\x64" // Shares (100)
      "\x00\x23\xB6\xF8" // Price (234.0600)
      ;
  std::size_t const bufsize = sizeof(buf) - 1;
  return std::make_pair(buf, bufsize);
}

std::pair<char const*, std::size_t> reg_sho_restriction() {
  static char const buf[] = u8"Y"      // Message Type
      JB_ITCH5_TEST_HEADER             // Common test header
                            "HSART   " // Stock
                            "0"        // Reg SHO Action
      ;
  std::size_t const bufsize = sizeof(buf) - 1;
  return std::make_pair(buf, bufsize);
}

std::pair<char const*, std::size_t> stock_directory() {
  static char const buf[] = u8"R"              // Message Type
      JB_ITCH5_TEST_HEADER                     // Common test header
                            "HSART   "         // Stock
                            "G"                // Market Category
                            "N"                // Financial Status Indicator
                            "\x00\x00\x00\x64" // Round Lot Size
                            "N"                // Round Lots Only
                            "O"                // Issue Classification
                            "C "               // Issue Sub-Type
                            "P"                // Authenticity
                            "N"                // Short Sale Threshold Indicator
                            "N"                // IPO Flag
                            "1"                // LULD Reference Price Tier
                            "N"                // ETP Flag
                            "\x00\x00\x00\x00" // ETP Leverage Factor
                            "N"                // Inverse Indicator
      ;
  std::size_t const bufsize = sizeof(buf) - 1;
  return std::make_pair(buf, bufsize);
}

std::pair<char const*, std::size_t> stock_trading_action() {
  static char const buf[] = u8"H"      // Message Type
      JB_ITCH5_TEST_HEADER             // Common test header
                            "HSART   " // Stock
                            "T"        // Trading State
                            "\x00"     // Reserved
                            "MWC1"     // Reason
      ;
  std::size_t const bufsize = sizeof(buf) - 1;
  return std::make_pair(buf, bufsize);
}

std::pair<char const*, std::size_t> system_event() {
  static char buf[] = u8"S" // Message Type
      JB_ITCH5_TEST_HEADER  // Common test header
                      "O"   // Event Code ("O" -> Start)
      ;
  std::size_t const size = sizeof(buf) / sizeof(buf[0]);
  return std::make_pair(buf, size);
}

std::pair<char const*, std::size_t> trade() {
  static char const buf[] = u8"P" // Message Type
      JB_ITCH5_TEST_HEADER        // Common test header
                            "\x00\x00\x00\x00"
                            "\x00\x00\x10\x92" // Order Reference Number (4242)
                            "B"                // Buy/Sell Indicator
                            "\x00\x00\x00\x64" // Shares (100)
                            "HSART   "         // Stock
                            "\x00\x12\xC6\xA4" // Price (123.0500)
                            "\x00\x00\x00\x00"
                            "\x00\x23\xB6\xF8" // Match Number (2340600)
      ;
  std::size_t const bufsize = sizeof(buf) - 1;
  return std::make_pair(buf, bufsize);
}

std::vector<char> create_message(int message_type, jb::itch5::timestamp ts,
                                 std::size_t total_size) {
  if (total_size < protocol::header_size or
      total_size > protocol::max_message_size) {
    std::ostringstream os;
    os << "ITCH-5.x messages size in bytes must be in the ["
       << protocol::header_size << "," << protocol::max_message_size
       << "] range";
    throw std::range_error(os.str());
  }

  std::vector<char> msg(total_size);
  if (message_type < 0 or message_type > 255) {
    std::ostringstream os;
    os << "out of range message type <" << message_type
       << "> valid range is [0,255]";
    throw std::range_error(os.str());
  }
  void* buf = &msg[0];
  // message type
  jb::itch5::encoder<true, std::uint8_t>::w(total_size, buf, 0, message_type);
  // stock locate
  jb::itch5::encoder<true, std::uint16_t>::w(total_size, buf, 1, 0);
  // tracking number
  jb::itch5::encoder<true, std::uint16_t>::w(total_size, buf, 3, 0);
  // timestamp
  jb::itch5::encoder<true, jb::itch5::timestamp>::w(total_size, buf, 5, ts);
  return msg;
}

} // namespace testing
} // namespace itch5
} // namespace jb
