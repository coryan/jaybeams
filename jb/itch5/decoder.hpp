#ifndef jb_itch5_decoder_hpp
#define jb_itch5_decoder_hpp

#include <jb/itch5/check_offset.hpp>

namespace jb {
namespace itch5 {

/**
 * Define the interface to decode ITCH-5.0 messages and message
 * fields.
 *
 * This class is specialized for every message type used to represent
 * ITCH-5.0 messages and their fields.
 *
 * @tparam validate if true the fields and message lengths are
 *   validated.  When false, the decoding is faster, but more likely to
 *   crash.
 * @tparam T the type of message or field to decode.
 *
 * TODO(#19) all this code should be replaced with Boost.Endian.
 */
template <bool validate, typename T>
struct decoder {
  /**
   * Read a single message or field.
   *
   * @param size the size of the message
   * @param msg the contents of the message
   * @param offset where in the message to start decoding
   *
   * @return An object of type T representing the contents of the
   * message.
   *
   * @throws std::runtime_error if the message cannot be parsed.
   */
  static T r(std::size_t size, void const* msg, std::size_t offset);
};

} // namespace itch5
} // namespace jb

#endif // jb_itch5_decoder_hpp
