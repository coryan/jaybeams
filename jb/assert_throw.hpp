#ifndef jb_assert_throw_hpp
#define jb_assert_throw_hpp

#include <stdexcept>

namespace jb {

/**
 * Raise std::exception to indicate an assertion failure
 */
[[noreturn]] void raise_assertion_failure(char const* filename, int lineno,
                                          char const* predicate);

#define JB_ASSERT_THROW(PRED)                                                  \
  do {                                                                         \
    if (not(PRED)) {                                                           \
      ::jb::raise_assertion_failure(__FILE__, __LINE__, #PRED);                \
    }                                                                          \
  } while (false)

} // namespace jb

#endif // jb_assert_throw_hpp
