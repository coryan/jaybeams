#ifndef jb_itch5_testing_data_hpp
#define jb_itch5_testing_data_hpp

#include <chrono>
#include <cstddef>
#include <utility>

namespace jb {
namespace itch5 {
namespace testing {

/// Return the expected timestamp for all the test messages
std::chrono::nanoseconds expected_ts();

//@{
/**
 * @name Well-known raw messages
 *
 * The following functions return well-known raw buffers for each
 * message type.  They are used during testing to validate the message
 * format.  They all return a buffer and its length.
 */
//@}
std::pair<char const*, std::size_t> message_header();
std::pair<char const*, std::size_t> add_order();
std::pair<char const*, std::size_t> add_order_mpid();
std::pair<char const*, std::size_t> broken_trade();
std::pair<char const*, std::size_t> cross_trade();
std::pair<char const*, std::size_t> ipo_quoting_period_update();
std::pair<char const*, std::size_t> market_participant_position();
std::pair<char const*, std::size_t> mwcb_breach();
std::pair<char const*, std::size_t> mwcb_decline_level();
std::pair<char const*, std::size_t> net_order_imbalance_indicator();
std::pair<char const*, std::size_t> order_cancel();
std::pair<char const*, std::size_t> order_delete();
std::pair<char const*, std::size_t> order_executed();
std::pair<char const*, std::size_t> order_executed_price();
std::pair<char const*, std::size_t> order_replace();
std::pair<char const*, std::size_t> reg_sho_restriction();
std::pair<char const*, std::size_t> stock_directory();
std::pair<char const*, std::size_t> stock_trading_action();
std::pair<char const*, std::size_t> system_event();
std::pair<char const*, std::size_t> trade();
//@}

} // namespace testing
} // namespace itch5
} // namespace jb

#endif // jb_itch5_testing_data_hpp
