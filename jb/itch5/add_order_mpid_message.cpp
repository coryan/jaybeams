#include <jb/itch5/add_order_mpid_message.hpp>

#include <iostream>

constexpr int jb::itch5::add_order_mpid_message::message_type;

std::ostream& jb::itch5::operator<<(
    std::ostream& os, add_order_mpid_message const& x) {
  return os << static_cast<add_order_message const&>(x)
            << ",attribution=" << x.attribution
      ;
}
