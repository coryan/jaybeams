#ifndef jb_testing_resize_if_application_hpp
#define jb_testing_resize_if_application_hpp

#include <type_traits>

namespace jb {
namespace testing {

/**
 * Type trait to evaluate if a collection of numbers (e.g. vector, multi_array)
 * is resizable.
 *
 * has_risize<C>::value is std::true_type if C is resizable.
 * std::false_type otherwise.
 *
 * @tparam C collection of numbers type to be evaluated if it is resizable
 */
template <typename C>
class has_resize {
private:
  template <typename T>
  static constexpr auto check(T*) -> typename std::is_void<
      decltype(std::declval<T>().resize(std::declval<std::size_t>()))>::type {
    return std::true_type();
  }

  template <typename T>
  static constexpr std::false_type check(...) {
    return std::false_type();
  }

public:
  using type = decltype(check<C>(nullptr));
  static constexpr bool value = type::value;
};

/**
 * Resize a timeseries to a newsize
 *
 * @param ts timeseries are any container representation (e.g. vector, deque),
 *   boost::multi_array, c-like array, etc
 * @param newsize size to resize the timeseries if possible
 * @tparam timeseries type of the timeseries collection representation
 */
template <typename timeseries>
void resize_if_applicable(timeseries& ts, std::size_t newsize, std::true_type) {
  ts.resize(newsize);
}

/**
 * Dummy specialization for non-resizable timeseries collections.
 *
 * @param ts timeseries are any container representation (e.g. vector, deque),
 *   boost::multi_array, c-like array, etc
 * @param newsize size to resize the timeseries if possible
 * @tparam timeseries type of the timeseries collection representation
 */
template <typename timeseries>
void resize_if_applicable(
    timeseries& ts, std::size_t newsize, std::false_type) {
}

/**
 * Resize a timeseries to a newsize if it is resizable
 *
 * @param ts timeseries are any container representation (e.g. vector, deque),
 *   boost::multi_array, c-like array, etc
 * @param newsize size to resize the timeseries if possible
 * @tparam timeseries type of the timeseries collection representation
 */
template <typename timeseries>
void resize_if_applicable(timeseries& ts, std::size_t newsize) {
  using has = typename has_resize<timeseries>::type;
  resize_if_applicable(ts, newsize, has());
}

} // namespace testing
} // namespace jb

#endif // jb_testing_resize_if_application_hpp
