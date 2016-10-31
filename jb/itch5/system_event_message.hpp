#ifndef jb_itch5_system_event_message_hpp
#define jb_itch5_system_event_message_hpp

#include <jb/itch5/char_list_field.hpp>
#include <jb/itch5/message_header.hpp>

namespace jb {
namespace itch5 {

/**
 * @typedef event_code_t
 *
 * Represent the 'Event Code' field on a 'System Event Message'
 */
typedef char_list_field<u'O', u'S', u'Q', u'M', u'E', u'C'> event_code_t;

/**
 * Represent a 'System Event Message' in the ITCH-5.0 protocol.
 */
struct system_event_message {
  constexpr static int message_type = u'S';

  message_header header;
  event_code_t event_code;
};

/// Specialize decoder for a jb::itch5::system_event_message
template <bool validate>
struct decoder<validate, system_event_message> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static system_event_message
  r(std::size_t size, void const* buf, std::size_t off) {
    system_event_message x;
    x.header = decoder<validate, message_header>::r(size, buf, off + 0);
    x.event_code = decoder<validate, event_code_t>::r(size, buf, off + 11);
    return x;
  }
};

/// Streaming operator for jb::itch5::system_event_message.
std::ostream& operator<<(std::ostream& os, system_event_message const& x);

} // namespace itch5
} // namespace jb

#endif // jb_itch5_system_event_message_hpp
