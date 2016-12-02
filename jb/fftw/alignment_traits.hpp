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
 */
template <typename T>
struct always_aligned : public std::false_type {};

template <typename T>
struct always_aligned<jb::fftw::aligned_vector<T>> : public std::true_type {};

template <typename T, std::size_t K>
struct always_aligned<jb::fftw::aligned_multi_array<T, K>>
    : public std::true_type {};

} // namespace fftw
} // namespace jb

#endif // jb_fftw_alignment_traits_hpp
