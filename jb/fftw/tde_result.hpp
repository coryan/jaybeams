#ifndef jb_fftw_tde_result_hpp
#define jb_fftw_tde_result_hpp

#include <jb/detail/array_traits.hpp>
#include <jb/fftw/aligned_multi_array.hpp>

namespace jb {
namespace fftw {

/**
 * tde_result is a type to handle the result of operating over the
 * last dimension array of a multi array of timeseries.
 * This is used when the operation returns a value out of the array,
 * therefore tde_result has one dimension less than the
 * tde_result constructor's argument.
 */

/**
 * Handles TDE result for container type timeseries.
 *
 * @tparam container_t container type timeseries
 * @tparam value_t type to store the result of the timeseries operation
 */
template <typename container_t, typename value_t>
class tde_result {
public:
  using value_type = value_t;
  using record_type = value_type;
  tde_result(container_t const& a)
      : value_{0}
      , size_{1} {
  }

  value_type& operator[](std::size_t pos) {
    return value_;
  }
  value_type const& operator[](std::size_t pos) const {
    return value_;
  }
  std::size_t size() const {
    return size_;
  }

private:
  record_type value_;
  std::size_t size_;
};

/**
 * Handles TDE result for a multi array type timeseries.
 *
 * @tparam T type of the value stored on the timeseries
 * @tparam K timeseries dimensionality
 * @tparam value_t type to store the result of the timeseries operation
 */
template <typename T, std::size_t K, typename value_t>
class tde_result<jb::fftw::aligned_multi_array<T, K>, value_t> {
public:
  using value_type = value_t;
  using array_type = jb::fftw::aligned_multi_array<T, K>;
  // multi array to store the result of the operation
  using record_type = jb::fftw::aligned_multi_array<value_type, K - 1>;

  tde_result(array_type const& a)
      : record_{jb::detail::array_shape(a)}
      , size_{jb::detail::element_count(a) / jb::detail::nsamples(a)} {
  }

  value_type& operator[](std::size_t pos) {
    return record_.data()[pos];
  }
  value_type const& operator[](std::size_t pos) const {
    return record_.data()[pos];
  }
  std::size_t size() const {
    return size_;
  }

private:
  record_type record_;
  std::size_t size_;
};

/**
 * Handles TDE result for a multi array type timeseries with
 * dimensionality 1.
 *
 * @tparam T type of the value stored on the timeseries
 * @tparam K timeseries dimensionality
 * @tparam value_t type to store the result of the timeseries operation
 */
template <typename T, typename value_t>
class tde_result<jb::fftw::aligned_multi_array<T, 1>, value_t> {
public:
  using value_type = value_t;
  using array_type = jb::fftw::aligned_multi_array<T, 1>;
  // result of the operation is a value
  using record_type = value_type;
  tde_result(array_type const& a)
      : value_{0}
      , size_{1} {
  }

  value_type& operator[](std::size_t pos) {
    return value_;
  }
  value_type const& operator[](std::size_t pos) const {
    return value_;
  }
  std::size_t size() const {
    return size_;
  }

private:
  record_type value_;
  std::size_t size_;
};

} // namespace fftw
} // namespace jb

#endif // jb_fftw_tde_result_hpp
