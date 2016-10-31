#ifndef jb_itch5_check_offset_hpp
#define jb_itch5_check_offset_hpp

#include <cstddef>

namespace jb {
namespace itch5 {

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
template <bool validate>
void check_offset(
    char const* msg, std::size_t size, std::size_t offset, std::size_t n) {
}

/// A version of jb::itch5::check_offset<> that actually validates.
template <>
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

#endif // jb_itch5_check_offset_hpp
