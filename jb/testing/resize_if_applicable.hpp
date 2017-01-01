#ifndef jb_testing_resize_if_application_hpp
#define jb_testing_resize_if_application_hpp

#include <type_traits>

namespace jb {
namespace testing {

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

template <typename timeseries>
void resize_if_applicable(timeseries& ts, std::size_t newsize, std::true_type) {
  ts.resize(newsize);
}

template <typename timeseries>
void resize_if_applicable(
    timeseries& ts, std::size_t newsize, std::false_type) {
}

template <typename timeseries>
void resize_if_applicable(timeseries& ts, std::size_t newsize) {
  using has = typename has_resize<timeseries>::type;
  resize_if_applicable(ts, newsize, has());
}

} // namespace testing
} // namespace jb

#endif // jb_testing_resize_if_application_hpp
