#ifndef jb_itch5_process_iostream_hpp
#define jb_itch5_process_iostream_hpp

#include <jb/itch5/process_iostream_mlist.hpp>

#include <jb/itch5/add_order_message.hpp>
#include <jb/itch5/add_order_mpid_message.hpp>
#include <jb/itch5/broken_trade_message.hpp>
#include <jb/itch5/cross_trade_message.hpp>
#include <jb/itch5/ipo_quoting_period_update_message.hpp>
#include <jb/itch5/market_participant_position_message.hpp>
#include <jb/itch5/mwcb_breach_message.hpp>
#include <jb/itch5/mwcb_decline_level_message.hpp>
#include <jb/itch5/net_order_imbalance_indicator_message.hpp>
#include <jb/itch5/order_cancel_message.hpp>
#include <jb/itch5/order_delete_message.hpp>
#include <jb/itch5/order_executed_message.hpp>
#include <jb/itch5/order_executed_price_message.hpp>
#include <jb/itch5/order_replace_message.hpp>
#include <jb/itch5/reg_sho_restriction_message.hpp>
#include <jb/itch5/stock_directory_message.hpp>
#include <jb/itch5/stock_trading_action_message.hpp>
#include <jb/itch5/system_event_message.hpp>
#include <jb/itch5/trade_message.hpp>
#include <jb/itch5/unknown_message.hpp>

namespace jb {
namespace itch5 {

#define KNOWN_ITCH5_MESSAGES                    \
  jb::itch5::add_order_message,                 \
  jb::itch5::add_order_mpid_message, \
  jb::itch5::broken_trade_message, \
  jb::itch5::cross_trade_message, \
  jb::itch5::ipo_quoting_period_update_message, \
  jb::itch5::market_participant_position_message, \
  jb::itch5::mwcb_breach_message, \
  jb::itch5::mwcb_decline_level_message, \
  jb::itch5::net_order_imbalance_indicator_message, \
  jb::itch5::order_cancel_message, \
  jb::itch5::order_delete_message, \
  jb::itch5::order_executed_message, \
  jb::itch5::order_executed_price_message, \
  jb::itch5::order_replace_message, \
  jb::itch5::reg_sho_restriction_message, \
  jb::itch5::stock_directory_message, \
  jb::itch5::stock_trading_action_message, \
  jb::itch5::system_event_message, \
  jb::itch5::trade_message

/**
 * Process an iostream of ITCH-5.0 messages.
 *
 * This is just a wrapper around jb::itch5::process_iostream_mlist()
 * using all the messages in ITCH-5.0 as the allowed message list.
 *
 * Please see @ref jb::itch5::message_handler_concept for a detailed
 * description of the message_handler requirements.
 */
template<typename message_handler>
void process_iostream(std::istream& in, message_handler& handler) {
  process_iostream_mlist<message_handler,KNOWN_ITCH5_MESSAGES>(in, handler);
}

#undef KNOWN_ITCH5_MESSAGES

} // namespace itch5
} // namespace jb

#endif // jb_itch5_process_iostream_hpp
