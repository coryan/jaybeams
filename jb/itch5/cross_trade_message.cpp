#include "jb/itch5/cross_trade_message.hpp"

#include <iostream>

constexpr int jb::itch5::cross_trade_message::message_type;

std::ostream& jb::itch5::
operator<<(std::ostream& os, cross_trade_message const& x) {
  return os << x.header << ",shares=" << x.shares << ",stock=" << x.stock
            << ",cross_price=" << x.cross_price
            << ",match_number=" << x.match_number
            << ",cross_type=" << x.cross_type;
}
