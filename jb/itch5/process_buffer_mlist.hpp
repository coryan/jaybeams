#ifndef jb_itch5_process_buffer_mlist_hpp
#define jb_itch5_process_buffer_mlist_hpp

#include <jb/itch5/decoder.hpp>
#include <jb/itch5/unknown_message.hpp>

#include <cstddef>

namespace jb {
namespace itch5 {

/**
 * Process a buffer with a single message: parse it and call the handler.
 *
 * As messages are received and broken down by
 * jb::itch5::process_iostream_mlist() they need to be parsed and then
 * the right member function on the message handler must be invoked.
 * Two partial specializations of this class implement this
 * functionality.  The fully generic version is, in fact, not
 * implemented and this is intentional.
 *
 * We use a class instead of a standalone function because partial
 * template specialization for functions is not allowed in C++11,
 * though it is trivially simulated, as show here.
 *
 * Please see @ref jb::itch5::message_handler_concept for a detailed
 * description of the message_handler requirements.
 */
template<typename message_handler, typename... message_types>
class process_buffer_mlist;

/**
 * Partial specialization for an empty list of messages.
 *
 * Call the handle_unknown() member function in the message handler.
 *
 * @tparam message_handler a type that meets the interface defined in
 * jb::itch5::message_handler_concept.
 */
template<typename message_handler>
class process_buffer_mlist<message_handler> {
 public:
  /**
   * Always call handle_unknown(), as the message type list is empty.
   *
   * @param handler a message handler per @ref
   *   jb::itch5::message_handler_concept.
   * @param recv_ts the timestamp when the message was received
   * @param msgcnt the number of messages received before this message
   * @param msgoffset the number of bytes received before this message
   * @param msgbuf the raw message buffer
   * @param msglen the raw message length
   */
  static void process(
      message_handler& handler,
      typename message_handler::time_point const& recv_ts,
      std::uint64_t msgcnt, std::size_t msgoffset,
      char const* msgbuf, std::size_t msglen) {
    handler.handle_unknown(
        recv_ts, jb::itch5::unknown_message(msgcnt, msgoffset, msglen, msgbuf));
  }
};

/**
 * Partial specialization for a list with at least one message.
 *
 * Recurse through the message list until it is parsed or handled by
 * the specialization for an empty message type list
 *
 * @tparam message_handler a type that meets the interface defined in
 * jb::itch5::message_handler_concept.
 * @tparam head_t the first message type in the message type list
 * @tparam tail_t the remaining message types in the message type list
 */
template<typename message_handler, typename head_t, typename... tail_t>
class process_buffer_mlist<message_handler,head_t,tail_t...> {
 public:
  /**
   * If any of the message types in the list matches the contents of
   * the buffer call handle_message() for that type in the handler.
   * Otherwise call handle_unknown().
   *
   * @param handler a message handler per @ref
   *   jb::itch5::message_handler_concept.
   * @param recv_ts the timestamp when the message was received
   * @param msgcnt the number of messages received before this message
   * @param msgoffset the number of bytes received before this message
   * @param msgbuf the raw message buffer
   * @param msglen the raw message length
   */
  static void process(
      message_handler& handler,
      typename message_handler::time_point const& recv_ts,
      std::uint64_t msgcnt, std::size_t msgoffset,
      char const* msgbuf, std::size_t msglen) {
    // if the message received matches the head then ...
    if (msgbuf[0] == head_t::message_type) {
      // ... parse the message ...
      head_t msg = jb::itch5::decoder<true,head_t>::r(msglen, msgbuf, 0);
      // ... the right handle_message() member function in the message
      // handler ...
      handler.handle_message(recv_ts, msgcnt, msgoffset, msg);
      return;
    }
    // ... recurse through the message list ...
    process_buffer_mlist<message_handler,tail_t...>::process(
        handler, recv_ts, msgcnt, msgoffset, msgbuf, msglen);
  }
};

} // namespace itch5
} // namespace jb

#endif // jb_itch5_process_buffer_mlist_hpp
