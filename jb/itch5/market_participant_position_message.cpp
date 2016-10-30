#include <jb/itch5/market_participant_position_message.hpp>

#include <iostream>

constexpr int jb::itch5::market_participant_position_message::message_type;

std::ostream& jb::itch5::
operator<<(std::ostream& os, market_participant_position_message const& x) {
  return os << x.header << ",mpid=" << x.mpid << ",stock=" << x.stock
            << ",primary_market_maker=" << x.primary_market_maker
            << ",market_maker_mode=" << x.market_maker_mode
            << ",market_participant_state=" << x.market_participant_state;
}
