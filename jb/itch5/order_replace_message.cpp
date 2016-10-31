#include "jb/itch5/order_replace_message.hpp"

#include <iostream>

constexpr int jb::itch5::order_replace_message::message_type;

std::ostream& jb::itch5::
operator<<(std::ostream& os, order_replace_message const& x) {
  return os << x.header << ",original_order_reference_number="
            << x.original_order_reference_number
            << ",new_order_reference_number=" << x.new_order_reference_number
            << ",shares=" << x.shares << ",price=" << x.price;
}
