#ifndef jb_itch5_decoder_hpp
#define jb_itch5_decoder_hpp

#include <cstddef>

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
 */
template<bool validate, typename T>
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
  static T r(std::size_t size, char const* msg, std::size_t offset);
};

/**
 * Verify that an offset and field length is valid (generic version).
 *
 * @tparam validate if true, actually validate the offset and length,
 * otherwise this is a no-op.
 *
 * @param msg a message to include in the exception (if any) raised
 *   when the validation fails.
 * @param size the size of the message
 * @param offset the offset into the message where we want to read
 *   data
 * @param n the size of the data we want to read
 *
 * @throws std::runtime_error if the validation fails.
 */
template<bool validate>
void check_offset(
    char const* msg, std::size_t size, std::size_t offset, std::size_t n) {
}

/// A version of jb::itch5::check_offset<> that actually validates.
template<>
void check_offset<true>(
    char const* msg, std::size_t size, std::size_t offset, std::size_t n);

/**
 * Convenience function to raise an exception upon a validation error.
 *
 * @throws std::runtime_error
 *
 * @param where a string representing what class, function or
 * component detected the problem.
 * @param what a string representing what value or expression failed
 * its validation.
 */
[[noreturn]] void raise_validation_failed(char const* where, char const* what);


} // namespace itch5
} // namespace jb

#endif /* jb_itch5_decoder_hpp */
