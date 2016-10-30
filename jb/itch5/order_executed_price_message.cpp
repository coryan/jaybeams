#include <jb/itch5/order_executed_price_message.hpp>

#include <iostream>

constexpr int jb::itch5::order_executed_price_message::message_type;

std::ostream& jb::itch5::operator<<(std::ostream& os,
                                    order_executed_price_message const& x) {
  return os << static_cast<order_executed_message const&>(x)
            << ",printable=" << x.printable
            << ",execution_price=" << x.execution_price;
}
