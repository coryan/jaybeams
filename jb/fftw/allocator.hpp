#ifndef jb_fftw_allocator_hpp
#define jb_fftw_allocator_hpp

#include <fftw3.h>
#include <limits>
#include <memory>

namespace jb {
namespace fftw {

/**
 * Define an allocator based on fftw_malloc()/fftw_free()
 *
 * FFTW3 provides functions to allocate memory aligned to whatever
 * requirements the vectorized instructions require.
 */
template <typename T>
class allocator {
public:
  typedef T value_type;
  typedef value_type* pointer;
  typedef value_type const* const_pointer;
  typedef void* void_pointer;
  typedef void const* const_void_pointer;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;
  template <typename U>
  struct rebind {
    typedef allocator<U> other;
  };

  pointer address(T& object) const {
    return &object;
  }
  const_pointer address(T const& object) const {
    return &object;
  }
  size_type max_size() const {
    return std::numeric_limits<std::size_t>::max();
  }
  template <typename... Args>
  void construct(pointer p, Args&&... args) {
    new (static_cast<void*>(p)) T(std::forward<Args>(args)...);
  }
  void destroy(pointer p) {
    p->~T();
  }
  pointer allocate(size_type count, const void* = 0) {
    return reinterpret_cast<T*>(fftw_malloc(count * sizeof(T)));
  }
  void deallocate(pointer p, size_type count) {
    fftw_free(static_cast<void*>(p));
  }

  bool operator==(allocator const& rhs) const {
    return true;
  }
  bool operator!=(allocator const& rhs) const {
    return not(*this == rhs);
  }
};

} // namespace fftw
} // namespace jb

#endif // jb_fftw_allocator_hpp
