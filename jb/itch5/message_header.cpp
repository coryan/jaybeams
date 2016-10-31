#include "jb/itch5/message_header.hpp"

#include <iostream>
#include <cctype>

std::ostream& jb::itch5::operator<<(std::ostream& os, message_header const& x) {
  if (std::isprint(x.message_type)) {
    os << "message_type=" << static_cast<char>(x.message_type);
  } else {
    os << "message_type=.(" << x.message_type << ")";
  }
  return os << ",stock_locate=" << x.stock_locate
            << ",tracking_number=" << x.tracking_number
            << ",timestamp=" << x.timestamp;
}
