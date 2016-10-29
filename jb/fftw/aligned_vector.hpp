#ifndef jb_fftw_aligned_vector_hpp
#define jb_fftw_aligned_vector_hpp

#include <jb/fftw/allocator.hpp>
#include <vector>

namespace jb {
namespace fftw {

/**
 * Alias std::vector with properly allocated storage for FFTW3.
 *
 * FFTW3 recommends using storage aligned for SIMD operations.
 * jb::fftw defines an STL allocator to take advantage of the FFTW3
 * memory allocation functions that guarantee this alignment.  This
 * type alias makes it easy to create std::vectors using the
 * allocator.
 */
template <typename T>
using aligned_vector = std::vector<T, jb::fftw::allocator<T>>;

} // namespace fftw
} // namespace jb

#endif // jb_fftw_aligned_vector_hpp
