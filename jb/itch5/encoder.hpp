#ifndef jb_itch5_encoder_hpp
#define jb_itch5_encoder_hpp

#include <jb/itch5/check_offset.hpp>

namespace jb {
namespace itch5 {

/**
 * TODO(#19) all this code should be replaced with Boost.Endian.
 */
template<bool validate, typename T>
struct encoder {
  /**
   * Write a single message or field to a buffer.
   *
   * @param size the size of the message
   * @param msg the contents of the message
   * @param offset where in the message to start encoding
   *
   * @throws std::runtime_error if the buffer cannot hold the message.
   */
  static void w(std::size_t size, void* msg, std::size_t offset, T const& x);
};

} // namespace itch5
} // namespace jb

#endif // jb_itch5_encoder_hpp
