#ifndef jb_fftw_aligned_multi_array_hpp
#define jb_fftw_aligned_multi_array_hpp

#include <jb/fftw/allocator.hpp>
#include <boost/multi_array.hpp>

namespace jb {
namespace fftw {

/**
 * Alias boost::multi_array with properly allocated storage for FFTW3.
 *
 * FFTW3 recommends using storage aligned for SIMD operations.
 * jb::fftw defines an STL allocator to take advantage of the FFTW3
 * memory allocation functions that guarantee this alignment.  This
 * type alias makes it easy to create boost::multi_arrays using the
 * allocator.
 *
 * @tparam value_type the type stored by the multi_array
 * @tparam num_dims the number of dimensions in the multi array
 */
template <typename value_type, std::size_t num_dims>
using aligned_multi_array =
    boost::multi_array<value_type, num_dims, jb::fftw::allocator<value_type>>;

} // namespace fftw
} // namespace jb

#endif // jb_fftw_aligned_multi_array_hpp
