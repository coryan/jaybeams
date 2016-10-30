#ifndef jb_as_hhmmss_hpp
#define jb_as_hhmmss_hpp

#include <chrono>
#include <iosfwd>

namespace jb {

/**
 * Helper class to print time durations in a HHMMSS.UUUUUU format.
 */
struct as_hhmmssu {
  /**
   * Constructor
   *
   * @param x a time duration in the units specified by the type
   *
   * @tparam time_duration_t an instance of std::chrono::duration, or
   * any type compatible with std::chrono::duration_cast<>
   */
  template <typename time_duration_t>
  explicit as_hhmmssu(time_duration_t const& x)
      : t(std::chrono::duration_cast<std::chrono::microseconds>(x)) {
  }

  std::chrono::microseconds t;
};

/// Format as_hhmmssu into an iostream
std::ostream& operator<<(std::ostream& os, as_hhmmssu const& x);

/**
 * Helper class to print time durations s in HHMMSS format.
 */
struct as_hhmmss {
  /**
   * Constructor
   *
   * @param x a time duration in the units specified by the type
   *
   * @tparam time_duration_t an instance of std::chrono::duration, or
   * any type compatible with std::chrono::duration_cast<>
   */
  template <typename time_duration_t>
  explicit as_hhmmss(time_duration_t const& x)
      : t(std::chrono::duration_cast<std::chrono::microseconds>(x)) {
  }

  std::chrono::microseconds t;
};

/// Format as_hhmmss into an iostream
std::ostream& operator<<(std::ostream& os, as_hhmmss const& x);

/**
 * Helper class to print time durations in HH:MM:SS.UUUUUU format.
 */
struct as_hh_mm_ss_u {
  /**
   * Constructor
   *
   * @param x a time duration in the units specified by the type
   *
   * @tparam time_duration_t an instance of std::chrono::duration, or
   * any type compatible with std::chrono::duration_cast<>
   */
  template <typename time_duration_t>
  explicit as_hh_mm_ss_u(time_duration_t const& x)
      : t(std::chrono::duration_cast<std::chrono::microseconds>(x)) {
  }

  std::chrono::microseconds t;
};

/// Format as_hh_mm_ss_u into an iostream
std::ostream& operator<<(std::ostream& os, as_hh_mm_ss_u const& x);

} // namespace jb

#endif // jb_as_hhmmss_hpp
