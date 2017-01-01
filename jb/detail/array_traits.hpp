#ifndef jb_detail_array_traits_hpp
#define jb_detail_array_traits_hpp

#include <boost/multi_array.hpp>
#include <vector>

namespace jb {
namespace detail {
/**
 * Define traits for array-like containers.
 *
 * The generic version implements the class assuming std::vector<> or
 * any similar container.  We provide specializations for other types
 * (such as boost::multi_array<>) to be able to operate on them
 * generically.
 *
 * @tparam container_type the type of container, typically std::vector<>
 */
template <typename container_type>
struct array_traits {
  /// Define the type of the elements in the container
  using element_type = typename container_type::value_type;
};

/// Count the number of elements for a vector-like container
template <typename container_type>
inline std::size_t element_count(container_type const& a) {
  return a.size();
}

/// Count the elements in the last dimension
template <typename container_type>
inline std::size_t nsamples(container_type const& a) {
  return a.size();
}

/// Return the shape of the array in a form suitable for construction
template <typename container_type>
inline std::size_t array_shape(container_type const& a) {
  return a.size();
}

/**
 * Define traits for boost::multi_array<> containers.
 */
template <typename T, std::size_t K, typename A>
struct array_traits<boost::multi_array<T, K, A>> {
  /// Define the type of the elements in the array
  using element_type = typename boost::multi_array<T, K, A>::element;
};

/// Count the number of elements
template <typename T, std::size_t K, typename A>
inline std::size_t element_count(boost::multi_array<T, K, A> const& a) {
  return a.num_elements();
}

/// Count the number of elements in the last dimension
template <typename T, std::size_t K, typename A>
inline std::size_t nsamples(boost::multi_array<T, K, A> const& a) {
  return a.shape()[a.num_dimensions() - 1];
}

/// Return the shape of the array in a form suitable for construction
template <typename T, std::size_t K, typename A>
inline std::vector<std::size_t>
array_shape(boost::multi_array<T, K, A> const& a) {
  return std::vector<std::size_t>(a.shape(), a.shape() + a.dimensionality);
}
} // namespace detail
} // namespace jb

#endif // jb_detail_array_traits_hpp
