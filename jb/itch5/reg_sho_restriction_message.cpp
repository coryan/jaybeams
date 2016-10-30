#include <jb/itch5/reg_sho_restriction_message.hpp>

#include <iostream>

constexpr int jb::itch5::reg_sho_restriction_message::message_type;

std::ostream& jb::itch5::operator<<(std::ostream& os,
                                    reg_sho_restriction_message const& x) {
  return os << x.header << ",stock=" << x.stock
            << ",reg_sho_action=" << x.reg_sho_action;
}
