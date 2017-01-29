#ifndef jb_detail_array_traits_hpp
#define jb_detail_array_traits_hpp

#include <jb/fftw/aligned_multi_array.hpp>
#include <jb/fftw/aligned_vector.hpp>

#include <boost/multi_array.hpp>
#include <vector>

namespace jb {
namespace detail {
/**
 * Alias element type stored on a container.
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

/**
 * Alias array_type based on the container_type shape
 * to store value_type.
 *
 * The generic version implements the class assuming std::vector<> or
 * any similar container.  We provide specializations for other types
 * (such as boost::multi_array<>) to be able to operate on them
 * generically.
 *
 * @tparam value_type the type to store on the aligned_container_type
 * @tparam array_type the type of container, typically std::vector<>
 */
template <typename value_type, typename container_type>
struct aligned_container {
  using array_type = typename jb::fftw::aligned_vector<value_type>;
};

/**
 * Count the number of elements for a vector-like container.
 *
 * The generic version implements the class assuming std::vector<> or
 * any similar container.
 *
 * @tparam container_type the type of container, typically std::vector<>
 */
template <typename container_type>
inline std::size_t element_count(container_type const& a) {
  return a.size();
}

/**
 * Count the elements in the last dimension of a vector-like container.
 *
 * The generic version implements the class assuming std::vector<> or
 * any similar container.
 *
 * @tparam container_type the type of container, typically std::vector<>
 */
template <typename container_type>
inline std::size_t nsamples(container_type const& a) {
  return a.size();
}

/**
 * Return the shape of the container in a form suitable for construction
 * of a vector-like container.
 *
 * The generic version implements the class assuming std::vector<> or
 * any similar container.
 *
 * @tparam container_type the type of container, typically std::vector<>
 */
template <typename container_type>
inline std::size_t array_shape(container_type const& a) {
  return a.size();
}

/**
 * Alias element type stored on a boost::multi_array<> containers.
 *
 * Provides specializations for a boost multi array container to be able
 * to operate on them generically.
 *
 * @tparam T the type stored by the boost::multi_array container
 * @tparam K boost multi array dimensionality
 * @tparam A an Allocator type for type T allocator storage
 */
template <typename T, std::size_t K, typename A>
struct array_traits<boost::multi_array<T, K, A>> {
  /// Define the type of the elements in the array
  using element_type = typename boost::multi_array<T, K, A>::element;
};

/**
 * Alias array_type based on the boost::multi_array type shape
 * to store value_type.
 *
 * Provides specializations for a boost multi array container to be able
 * to operate on them generically.
 *
 * @tparam value_type the type to store on the aligned_container_type
 * @tparam T the type stored by the boost::multi_array container
 * @tparam K boost multi array dimensionality
 * @tparam A an Allocator type for type T allocator storage
 */
template <typename value_type, typename T, std::size_t K, typename A>
struct aligned_container<value_type, boost::multi_array<T, K, A>> {
  /// Define the type of the elements in the container
  using array_type = typename jb::fftw::aligned_multi_array<value_type, K>;
};

/**
 * Count the number of elements for boost::multi_array<> containers.
 *
 * Provides specializations for a boost multi array container to be able
 * to operate on them generically.
 *
 * @tparam T the type stored by the boost::multi_array container
 * @tparam K boost multi array dimensionality
 * @tparam A an Allocator type for type T allocator storage
 */
template <typename T, std::size_t K, typename A>
inline std::size_t element_count(boost::multi_array<T, K, A> const& a) {
  return a.num_elements();
}

/**
 * Count the number of elements in the last dimension for
 * boost::multi_array<> containers.
 *
 * Provides specializations for a boost multi array container to be able
 * to operate on them generically.
 *
 * @tparam T the type stored by the boost::multi_array container
 * @tparam K boost multi array dimensionality
 * @tparam A an Allocator type for type T allocator storage
 */
template <typename T, std::size_t K, typename A>
inline std::size_t nsamples(boost::multi_array<T, K, A> const& a) {
  return a.shape()[a.num_dimensions() - 1];
}

/**
 * Return the shape of the container in a form suitable for construction
 * of a boost::multi_array<> containers.
 *
 * Provides specializations for a boost multi array container to be able
 * to operate on them generically.
 *
 * @tparam T the type stored by the boost::multi_array container
 * @tparam K boost multi array dimensionality
 * @tparam A an Allocator type for type T allocator storage
 */
template <typename T, std::size_t K, typename A>
inline std::vector<std::size_t>
array_shape(boost::multi_array<T, K, A> const& a) {
  return std::vector<std::size_t>(a.shape(), a.shape() + a.dimensionality);
}

} // namespace detail
} // namespace jb

#endif // jb_detail_array_traits_hpp
