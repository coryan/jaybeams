#include "jb/itch5/mwcb_breach_message.hpp"

#include <iostream>

constexpr int jb::itch5::mwcb_breach_message::message_type;

std::ostream& jb::itch5::operator<<(std::ostream& os,
                                    mwcb_breach_message const& x) {
  return os << x.header << ",breached_level=" << x.breached_level;
}
