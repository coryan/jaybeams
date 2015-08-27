#ifndef jb_itch5_process_iostream_mlist_hpp
#define jb_itch5_process_iostream_mlist_hpp

#include <jb/itch5/process_buffer_mlist.hpp>
#include <jb/itch5/base_decoders.hpp>
#include <jb/log.hpp>

namespace jb {
namespace itch5 {

/*
 * Process an iostream of ITCH-5.0 given a list of expected messages.
 *
 * This function parses each ITCH-5.0 message in an iostream, then
 * calls a handler for the message.  The messages are allocated on the
 * stack, and do not survice the call to the handler.
 *
 * If the iostream contains a message that is not on the list, the
 * message is not parsed and the handle_unknown() member function of
 * the handler can be ignored.  This is convenient when you want to
 * ignore the most common messages in the protocol, and only process a
 * subset.  It also decouples the list of messages from the processing.
 *
 * Please see @ref jb::itch5::message_handler_concept for a detailed
 * description of the message_handler requirements.
 */
template<typename message_handler, typename... message_types>
void process_iostream_mlist(std::istream& is, message_handler& handler) {
  std::size_t msgoffset = 0;
  for (long msgcnt = 0; is.good(); ++msgcnt) {
    char blen[2];
    is.read(blen, 2);
    if (not is) {
      if (not is.eof()) {
        JB_LOG(error) << "reading length when msgcnt=" << msgcnt
                      << ", msgoffset=" << msgoffset;
      }
      return;
    }
    msgoffset += 2;

    constexpr std::size_t maxmsglen = 1L<<16;
    std::size_t msglen = jb::itch5::decoder<true,std::uint16_t>::r(
        2, blen, 0);
    char msgbuf[maxmsglen];
    is.read(msgbuf, msglen);
    auto recv_ts = handler.now();
    process_buffer_mlist<message_handler,message_types...>::process(
        handler, recv_ts, msgcnt, msgoffset, msgbuf, msglen);
    msgoffset += msglen;
  }
}

} // namespace itch5
} // namespace jb

#endif // jb_itch5_process_iostream_mlist_hpp
