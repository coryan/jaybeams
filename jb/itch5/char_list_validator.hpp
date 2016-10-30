#ifndef jb_itch5_char_list_validator_hpp
#define jb_itch5_char_list_validator_hpp

namespace jb {
namespace itch5 {

/**
 * Define a functor to validate character fields with limited values.
 *
 * Many ITCH-5.0 fields are single character wide (on the wire) and
 * are only supposed to take a limited set of values.  We want a
 * functor to validate these fields against their list of values, but
 * also to disable the validation if not needed.
 *
 * Typically this validators would be used as:
 *   typedef char_list_validator<true,u'A',u'B',u'C'> validator;
 * or
 *   typedef char_list_validator<false,u'A',u'B',u'C'> validator;
 *
 * The first template parameter defines if the validator actually
 * performs any checking at all, in production one probably wants its
 * value to be 'false'.  The remaining template parameters define the
 * list of possible values to be accepted.
 */
template <bool validate, int... V> struct char_list_validator {
  void operator()(int x) const {
  }
};

/// Helper function, raises an exception describing a mismatched value.
[[noreturn]] void char_list_validation_failed(int x);

/**
 * Specialize for the empty list.
 *
 * All values are rejected in this case.
 */
template <> struct char_list_validator<true> {
  void operator()(int x) const {
    char_list_validation_failed(x);
  }
};

/**
 * Recursively define the validator for the enabled case.
 */
template <int a, int... V> struct char_list_validator<true, a, V...> {
  void operator()(int x) const {
    if (a == x) {
      return;
    }
    char_list_validator<true, V...> f;
    f(x);
  }
};

} // namespace itch5
} // namespace jb

#endif // jb_itch5_char_list_validator_hpp
