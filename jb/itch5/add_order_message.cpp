#include "jb/itch5/add_order_message.hpp"

#include <iostream>

constexpr int jb::itch5::add_order_message::message_type;

std::ostream& jb::itch5::
operator<<(std::ostream& os, add_order_message const& x) {
  return os << x.header
            << ",order_reference_number=" << x.order_reference_number
            << ",buy_sell_indicator=" << x.buy_sell_indicator
            << ",shares=" << x.shares << ",stock=" << x.stock
            << ",price=" << x.price;
}
