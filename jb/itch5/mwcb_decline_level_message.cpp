#include "jb/itch5/mwcb_decline_level_message.hpp"

#include <iostream>

constexpr int jb::itch5::mwcb_decline_level_message::message_type;

std::ostream& jb::itch5::operator<<(std::ostream& os,
                                    mwcb_decline_level_message const& x) {
  return os << x.header << ",level_1=" << x.level_1 << ",level_2=" << x.level_2
            << ",level_3=" << x.level_3;
}
