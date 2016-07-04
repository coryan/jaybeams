#ifndef jb_itch5_p2ceil_hpp
#define jb_itch5_p2ceil_hpp

#include <cstdint>
#include <type_traits>

namespace jb {
namespace itch5 {

/**
 * Implement the key operation in the p2ceil() function.
 *
 * The p2ceil() functions are implemented as a series of recursive
 * calls to this function.
 *
 * @tparam T the type of the input, must be an integer type.
 */
template<typename T>
constexpr T p2ceil_kernel(int shift, T n) {
  static_assert(
      std::is_integral<T>::value, "p2ceil_kernel input type must be integral");
  return n | (n >> shift);
}

/**
 * Find the smallest power of 2 larger than n for a 64-bit integer.
 *
 * The algorithm can be executed at compile time, so it is suitable
 * for use in template expressions.  The algorithm first computes all
 * the 
 *
 * @param n the input number, must be smaller or equal to 2^63.
 * @returns the smallest power of two larger than @a n.
 */
constexpr std::uint64_t p2ceil(std::uint64_t n) {
  // Picture the bitwise representation of the number.  Let {b} be the
  // highest bit set on the number.  If we compute:
  //   n1 = n | n >> 1;
  // we have a number where (at least), {b} and {b-1} are set.  After
  // computing this, we can compute:
  //   n2 = n1 | n1 >> 2;
  // to obtain a number where at least {b} through {b-3} are set.
  //
  // We can repeatedly apply this operation with a 4-shift, 8-shift,
  // 16-shift and 32-shift to obtain a number where all the bits from
  // {b} to 0 are set.  That number is equal to 2^{b+1}-1, so adding 1
  // to this number is exactly what we need.
  //
  // Because we are performing this operations in a constexpr (under
  // C++11), we cannot use intermediate variables, but we can use
  // recursive calls to other constexpr functions:
  return 1 + p2ceil_kernel(
      32, p2ceil_kernel(
          16, p2ceil_kernel(
              8, p2ceil_kernel(
                  4, p2ceil_kernel(
                      2, p2ceil_kernel(1, n))))));
}

/**
 * Find the smallest power of 2 larger than n for a 32-bit integer.
 *
 * @param n the input number, must be smaller or equal to 2^31.
 * @returns the smallest power of two larger than @a n.
 */
constexpr std::uint32_t p2ceil(std::uint32_t n) {
  return 1 + p2ceil_kernel(
      16, p2ceil_kernel(
          8, p2ceil_kernel(
              4, p2ceil_kernel(
                  2, p2ceil_kernel(1, n)))));
}

/**
 * Find the smallest power of 2 larger than n for a 16-bit integer.
 *
 * @param n the input number, must be smaller or equal to 2^15.
 * @returns the smallest power of two larger than @a n.
 */
constexpr std::uint16_t p2ceil(std::uint16_t n) {
  return 1 + p2ceil_kernel(
      8, p2ceil_kernel(
          4, p2ceil_kernel(
              2, p2ceil_kernel(1, n))));
}

/**
 * Find the smallest power of 2 larger than n for an 8-bit integer.
 *
 * @param n the input number, must be smaller or equal to 2^7.
 * @returns the smallest power of two larger than @a n.
 */
constexpr std::uint8_t p2ceil(std::uint8_t n) {
  return 1 + p2ceil_kernel(
      4, p2ceil_kernel(
          2, p2ceil_kernel(1, n)));
}

} // namespace itch5
} // namespace jb

#endif // jb_itch5_p2ceil_hpp
