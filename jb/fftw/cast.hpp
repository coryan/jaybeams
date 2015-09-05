#ifndef jb_fftw_cast_hpp
#define jb_fftw_cast_hpp

#include <jb/fftw/traits.hpp>

namespace jb {
namespace fftw {

template<typename F>
typename traits<F>::fftw_complex_type*
fftw_cast_array(std::complex<F>* ptr) {
  return reinterpret_cast<typename traits<F>::fftw_complex_type*>(ptr);
}

template<typename F>
typename traits<F>::fftw_complex_type const*
fftw_cast_array(std::complex<F> const* ptr) {
  return reinterpret_cast<typename traits<F>::fftw_complex_type const*>(ptr);
}

template<typename F>
F* fftw_cast_array(F* ptr) {
  return ptr;
}

template<typename F>
F const* fftw_cast_array(F const* ptr) {
  return ptr;
}

template<typename vector>
auto fftw_cast(vector& in) -> decltype(fftw_cast_array(&in[0])) {
  return fftw_cast_array(&in[0]);
}

template<typename vector>
auto fftw_cast(vector const& in) -> decltype(fftw_cast_array(&in[0])) {
  return fftw_cast_array(&in[0]);
}

} // namespace fftw
} // namespace jb

#endif // jb_fftw_cast_hpp
