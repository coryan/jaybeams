#ifndef jb_testing_check_tde_result_close_enough_hpp
#define jb_testing_check_tde_result_close_enough_hpp

#include <jb/fftw/tde_result.hpp>
#include <jb/testing/check_complex_close_enough.hpp>

#include <boost/test/unit_test.hpp>

namespace jb {
namespace testing {

/**
 * Compare tde_result value within some tolerance.
 *
 * @return true if the numbers are within tolerance.
 *
 * @param actual the first tde_result to compare
 * @param expected the second tde_result to compare
 * @param tol expressed in:
 * - "integer" if actual value type is integer
 * - "number of epsilons" otherwise
 *
 * @tparam T the type of tde_result(s) to compare 
 */
template <typename T>
bool tde_result_close_enough(T actual, T expected, int tol) {
  using tde_result_type = T;
  using value_type = typename tde_result_type::value_type;
  if (std::numeric_limits<value_type>::is_integer) {
    value_type value_tol = static_cast<value_type>(tol);
    for (std::size_t i=0; i != actual.size(); ++i) {
      if ( actual[i] == expected[i] ) {
	continue;
      }
      if ( (actual[i] < expected[i] - value_tol)
	   or (actual[i] > expected[i] + value_tol) ) {
        return false;
      }
    }
    return true;
  }
  // it is not integer value type,
  // let's allow check_complex_close_enough to handle it...
  for (std::size_t i=0; i != actual.size(); ++i) {
    if ( not close_enough(actual[i], expected[i], tol)) {
      return false;
    }
  }
  return true;
}
  
} // namespace testing
} // namespace jb

#endif // jb_testing_check_tde_result_close_enough_hpp
