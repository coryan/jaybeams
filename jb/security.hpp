#ifndef jb_security_hpp
#define jb_security_hpp

#include <jb/security_directory.hpp>
#include <jb/security_directory_entry.hpp>

#include <boost/operators.hpp>

#include <memory>
#include <mutex>

namespace jb {

/**
 * Define the interface for a security.
 *
 * Applications that work on market data often need to track
 * variable amounts of information about the securities they work
 * with.  This is the main interface to read the attributes of a
 * security.  Please read the documentation for jb::security_directory
 * for more details about the motivation.
 */
class security : public boost::less_than_comparable<security, security>,
                 public boost::equality_comparable<security, security> {
public:
  /// Create an invalid security, associated to no directory
  security();

  /// Copy constructor
  security(security const& rhs)
      : directory_(rhs.directory_)
      , id_(rhs.id_)
      , generation_(rhs.generation_.load())
      , entry_(rhs.entry_) {
  }
  
  /// Return the string representation of the security
  std::string const& str() const {
    check_directory(__func__);
    return entry_->symbol;
  }

  /// Get the value of a static attribute
  template <typename attribute>
  typename attribute::value_type const& get() const {
    check_directory(__func__);
    return directory_->get_attribute<attribute>(*this);
  }

  bool operator<(security const& rhs) const {
    return id_ < rhs.id_;
  }
  bool operator==(security const& rhs) const {
    return id_ == rhs.id_;
  }

private:
  // Grant access to the security container
  friend class security_directory;

  /// Constructor
  security(
      std::shared_ptr<security_directory> ptr, int id, unsigned int generation,
      std::shared_ptr<security_directory_entry> entry)
      : directory_(ptr)
      , id_(id)
      , generation_(generation)
      , entry_(entry) {
  }

  /**
   * Check that the directory_ shared pointer is set.
   *
   * @throws std::runtime_exception if directory_ is null
   * @param function_name the name of the function checking
   */
  void check_directory(char const* function_name) const;

private:
  std::shared_ptr<security_directory> directory_;
  int id_;
  mutable std::atomic<unsigned int> generation_;
  mutable std::shared_ptr<security_directory_entry> entry_;
};

} // namespace jb

#endif // jb_security_hpp
