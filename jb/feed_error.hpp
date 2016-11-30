#ifndef jb_feed_error_hpp
#define jb_feed_error_hpp

#include <stdexcept>

namespace jb {

/**
 * Communicate feed error exceptions.
 */
class feed_error : public std::runtime_error {
public:
  feed_error(std::string const& msg)
      : std::runtime_error("Feed error: " + msg) {
  }
};

} // namespace jb

#endif // jb_feed_error_hpp
