#include "jb/itch5/order_executed_message.hpp"

#include <iostream>

constexpr int jb::itch5::order_executed_message::message_type;

std::ostream& jb::itch5::
operator<<(std::ostream& os, order_executed_message const& x) {
  return os << x.header
            << ",order_reference_number=" << x.order_reference_number
            << ",executed_shares=" << x.executed_shares
            << ",match_number=" << x.match_number;
}
