#ifndef jb_usage_hpp
#define jb_usage_hpp

#include <stdexcept>

namespace jb {

/**
 * A simple class to communicate the result of parsing the options.
 */
class usage : public std::runtime_error {
 public:
  usage(std::string const& msg, int e)
      : std::runtime_error(msg)
      , exit_(e) {
  }
  usage(char const* msg, int e)
      : usage(std::string(msg), e) {
  }

  int exit_status() const {
    return exit_;
  }

 private:
  // The exit status for the program.
  int exit_;
};

} // namespace jb

#endif // jb_usage_hpp
