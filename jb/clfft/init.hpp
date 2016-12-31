#ifndef jb_clfft_init_hpp
#define jb_clfft_init_hpp

#include <clFFT.h>

namespace jb {
namespace clfft {

/**
 * Wrap clfftPlanHandle objects in a class that can automatically
 * destroy them.
 */
class init {
public:
  init();
  ~init();

  //@{
  /**
   * @name Deleted functions
   */
  init(init&&) = delete;
  init(init const&) = delete;
  init& operator=(init const&) = delete;
  //@}
};

} // namespace clfft
} // namespace jb

#endif // jb_clfft_init_hpp
