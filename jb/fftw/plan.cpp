#include "jb/fftw/plan.hpp"

#include <sstream>
#include <stdexcept>

namespace jb {
namespace fftw {

void check_create_plan_inputs(
    std::size_t in_elements, std::size_t on_elements, std::size_t in_nsamples,
    std::size_t on_nsamples, char const* function_name) {
  if (in_nsamples != on_nsamples) {
    std::ostringstream os;
    os << "mismatched number of samples (" << in_nsamples
       << " != " << on_nsamples << ") in " << function_name;
    throw std::invalid_argument(os.str());
  }
  if (in_elements != on_elements) {
    std::ostringstream os;
    os << "mismatched element count (" << in_elements << " != " << on_elements
       << ") in " << function_name;
    throw std::invalid_argument(os.str());
  }
  if (in_nsamples == 0) {
    std::ostringstream os;
    os << "nsamples must be non-zero in " << function_name;
    throw std::invalid_argument(os.str());
  }
}

void check_create_plan_inputs(
    std::size_t in_elements, std::size_t on_elements,
    char const* function_name) {
  if (in_elements != on_elements) {
    std::ostringstream os;
    os << "mismatched vector sizes (" << in_elements << " != " << on_elements
       << ") in " << function_name;
    throw std::invalid_argument(os.str());
  }
}

} // namespace fftw
} // namespace jb
