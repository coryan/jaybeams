#ifndef jb_itch5_noop_validator_hpp
#define jb_itch5_noop_validator_hpp

namespace jb {
namespace itch5 {

/**
 * A validator functor that accepts all values.
 *
 * @tparam T the type of object to validate.
 */
template<typename T>
struct noop_validator {
  /**
   * Validate the object.
   *
   * @returns Always true.
   */
  inline bool operator()(T const&) const {
    return true;
  }
};

} // namespace itch5
} // namespace jb

#endif // jb_itch5_noop_validator_hpp
