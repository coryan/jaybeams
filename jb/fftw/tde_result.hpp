#ifndef jb_fftw_tde_result_hpp
#define jb_fftw_tde_result_hpp

#include <jb/detail/array_traits.hpp>
#include <jb/fftw/aligned_multi_array.hpp>

namespace jb {
namespace fftw {

/**
 * A time-delay estimator (TDE) is an algorithm to compare
 * two families of timeseries and return the estimated delay
 * of the first family vs. the second family.
 * When the families of timeseries are represented by an array
 * of dimension K, the last dimension is interpreted as time,
 * and the remaining K-1 dimensions are interpreted as the
 * family parameters.
 * In that case, the output of a TDE is an array of one dimension
 * less than the inputs to the TDE algorithm.
 *
 * In contrast, when the input into the TDE algorithm is a simple
 * vector, or a one dimensional array then the output is a simple value.
 *
 * This template class, and its specializations, compute the representation for
 * the TDE result given the input type.  Because the TDE algorithms usually have
 * two outputs (the time delay estimate, usually represented by an integer,
 * and the confidence, usually a floating point type), the class is parametric
 * on the output type. This parametrization allows us to use this type in other
 * timeseries computations that return a (1) value out of operating on the last
 * dimension (e.g. sum sqr, average).
 *
 * @tparam container_t the representation for a timeseries, expected to be
 * a one-dimensional container such as std::vector<> or std::deque<>
 * @tparam value_t type to store the result of the timeseries operation
 */
template <typename container_t, typename value_t>
class tde_result {
public:
  using value_type = value_t;
  using record_type = value_type;
  tde_result(container_t const& a)
      : value_{0} {
  }

  /// tde_result holds only one value, we overload the subscription operator
  /// to allow generic usage of the type.
  value_type& operator[](std::size_t pos) {
    return value_;
  }

  /// tde_result value held by a const object.
  /// tde_result holds only one value, we overload the subscription operator
  /// to allow generic usage of the type.
  value_type const& operator[](std::size_t pos) const {
    return value_;
  }

  /// size of tde_result record_type, holds only one value
  std::size_t size() const {
    return 1;
  }

private:
  record_type value_;
};

/**
 * Handles TDE result for a multi array type timeseries.
 *
 * @tparam T type of the value stored on the timeseries
 * @tparam K timeseries dimensionality
 * @tparam A an Allocator type for type T allocator storage
 * @tparam value_t type to store the result of the timeseries computation
 */
template <typename T, std::size_t K, typename A, typename value_t>
class tde_result<boost::multi_array<T, K, A>, value_t> {
public:
  using value_type = value_t;
  using array_type = boost::multi_array<T, K, A>;
  // multi array to store the result of the timeseries computation
  using record_type = jb::fftw::aligned_multi_array<value_type, K - 1>;

  /// constructor based on a multi_array of dimensionality K
  /// the last dimension is ignored
  /// size is reduced by the elements of the last dimension (nsamples)
  tde_result(array_type const& a)
      : record_{jb::detail::array_shape(a)} {
  }

  /// tde_result holds a multi array, we overload the subscription operator
  /// to allow generic usage of the type.
  value_type& operator[](std::size_t pos) {
    return record_.data()[pos];
  }

  /// tde_result value held by a const object.
  /// tde_result holds a multi array, we overload the subscription operator
  /// to allow generic usage of the type.
  value_type const& operator[](std::size_t pos) const {
    return record_.data()[pos];
  }

  /// size is number of elements stored on the multi array record_
  std::size_t size() const {
    return jb::detail::element_count(record_);
  }

private:
  record_type record_;
};

/**
 * Handles TDE result for a multi array type timeseries with
 * dimensionality 1.
 *
 * @tparam T type of the value stored on the timeseries
 * @tparam A an Allocator type for type T allocator storage
 * @tparam value_t type to store the result of the timeseries operation
 */
template <typename T, typename A, typename value_t>
class tde_result<boost::multi_array<T, 1, A>, value_t> {
public:
  using value_type = value_t;
  using array_type = jb::fftw::aligned_multi_array<T, 1>;
  // result of the operation is one (1) value
  using record_type = value_type;

  /// constructor based on a array_type of dimensionaltity 1
  tde_result(array_type const& a)
      : value_{0} {
  }

  /// tde_result holds only one value, we overload the subscription operator
  /// to allow generic usage of the type.
  value_type& operator[](std::size_t pos) {
    return value_;
  }

  /// tde_result value held by a const object.
  /// tde_result holds only one value, we overload the subscription operator
  /// to allow generic usage of the type.
  value_type const& operator[](std::size_t pos) const {
    return value_;
  }

  /// size of tde_result record_type, holds only one value
  std::size_t size() const {
    return 1;
  }

private:
  record_type value_;
};

} // namespace fftw
} // namespace jb

#endif // jb_fftw_tde_result_hpp
