#include "jb/itch5/stock_trading_action_message.hpp"

#include <iostream>

constexpr int jb::itch5::stock_trading_action_message::message_type;

std::ostream& jb::itch5::
operator<<(std::ostream& os, stock_trading_action_message const& x) {
  return os << x.header << ",stock=" << x.stock
            << ",trading_state=" << x.trading_state
            << ",reserved=" << x.reserved << ",reason=" << x.reason;
}
