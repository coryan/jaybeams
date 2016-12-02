#ifndef jb_security_directory_entry_hpp
#define jb_security_directory_entry_hpp

#include <jb/security_attributes.hpp>
#include <memory>
#include <string>

namespace jb {

/// A tag to distinguish the security_directory attributes
struct security_directory_tag {};

/// Define the type used to store static security attributes
using security_directory_attributes =
    security_attributes<security_directory_tag>;

/**
 * The contents of a directory entry
 */
struct security_directory_entry {
  /// The id of the security
  int id;
  /// The symbol (aka ticker) of the security
  std::string symbol;

  /// The attributes of the security
  std::shared_ptr<security_directory_attributes> attributes;
};

} // namespace jb

#endif // jb_security_directory_entry_hpp
