#ifndef jb_fftw_alignment_traits_hpp
#define jb_fftw_alignment_traits_hpp

#include <jb/fftw/aligned_multi_array.hpp>
#include <jb/fftw/aligned_vector.hpp>

#include <type_traits>

namespace jb {
namespace fftw {
/**
 * Determine if a timeseries type guarantees alignment suitable for
 * SIMD optimizations.
 *
 * We provide specializations for aligned_vector (e.g. std::vector<>) and
 * aligned_multi_array (e.g. boost::multi_array) to be able to operate
 * on them generically.
 *
 * @tparam T the type stored by the timeseries container
 */
template <typename T>
struct always_aligned : public std::false_type {};

/**
 * Determine if a aligned_vector timeseries type guarantees alignment suitable
 * for
 * SIMD optimizations.
 *
 * @tparam T the type stored by the aligned_vector timeseries
 */
template <typename T>
struct always_aligned<jb::fftw::aligned_vector<T>> : public std::true_type {};

/**
 * Determine if a aligned_multi_array timeseries type guarantees alignment
 * suitable for
 * SIMD optimizations.
 *
 * @tparam T the type stored by the aligned_multi_array timeseries
 * @tparam K aligned_multi_array dimensionality
 *
 */
template <typename T, std::size_t K>
struct always_aligned<jb::fftw::aligned_multi_array<T, K>>
    : public std::true_type {};

} // namespace fftw
} // namespace jb

#endif // jb_fftw_alignment_traits_hpp
