#ifndef jb_testing_create_random_sample_hpp
#define jb_testing_create_random_sample_hpp

#include <complex>

namespace jb {
namespace testing {

/**
 * Wrap a random number generator so it can be used for both real and
 * complex numbers.
 */
template<typename T>
struct create_random_sample {
  /// Return a new number using the generator
  template<typename generator>
  T operator()(generator& gen) const {
    return gen();
  }
};

/**
 * Specialize create_random_sample for complex numbers.
 */
template<typename T>
struct create_random_sample<std::complex<T>> {
  /// Return a new complex number using the generator
  template<typename generator>
  std::complex<T> operator()(generator& gen) const {
    return std::complex<T>(gen(), gen());
  }
};

} // namespace testing
} // namespace jb

#endif // jb_testing_create_random_sample_hpp
