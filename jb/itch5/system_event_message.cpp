#include "jb/itch5/system_event_message.hpp"

#include <iostream>

constexpr int jb::itch5::system_event_message::message_type;

std::ostream& jb::itch5::operator<<(std::ostream& os,
                                    system_event_message const& x) {
  return os << x.header << ",event_code=" << x.event_code;
}
