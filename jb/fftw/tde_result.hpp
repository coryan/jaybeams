#ifndef jb_fftw_tde_result_hpp
#define jb_fftw_tde_result_hpp

namespace jb {
namespace fftw {

/**
 * tde_result is a type to handle the result of an operation over
 * a timeseries that generates a value out of the n samples.
 * In the case of a multi array the last dimension is elminated
 */

/// handles TDE result for container type timeseries
template <typename container_t, typename value_t>
class tde_result {
public:
  using record_type = value_t;
  tde_result(container_t const& a)
      : value_{0} {
  }

  value_t& operator[](std::size_t pos) {
    return value_;
  }
  value_t const& operator[](std::size_t pos) const {
    return value_;
  }

private:
  record_type value_;
};

/// handles TDE result for array_type timeseries
template <typename T, std::size_t K, typename value_t>
class tde_result<jb::fftw::aligned_multi_array<T, K>, value_t> {
public:
  using array_type = jb::fftw::aligned_multi_array<T, K>;
  using record_type = jb::fftw::aligned_multi_array<value_t, K - 1>;

  tde_result(array_type const& a)
      : record_{jb::detail::array_shape(a)} {
  }

  value_t& operator[](std::size_t pos) {
    return record_.data()[pos];
  }
  value_t const& operator[](std::size_t pos) const {
    return record_.data()[pos];
  }

private:
  record_type record_;
};

/// handles TDE result for timeseries with dimensionality = 1
template <typename T, typename value_t>
class tde_result<jb::fftw::aligned_multi_array<T, 1>, value_t> {
public:
  using array_type = jb::fftw::aligned_multi_array<T, 1>;
  using record_type = value_t;
  tde_result(array_type const& a)
      : value_{0} {
  }

  value_t& operator[](std::size_t pos) {
    return value_;
  }
  value_t const& operator[](std::size_t pos) const {
    return value_;
  }

private:
  record_type value_;
};

} // namespace fftw
} // namespace jb

#endif // jb_fftw_tde_result_hpp
