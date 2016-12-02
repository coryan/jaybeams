#ifndef jb_security_directory_hpp
#define jb_security_directory_hpp

#include <jb/security_directory_entry.hpp>

#include <map>
#include <mutex>

namespace jb {

class security;

/**
 * Maintain a collection of securities and their attributes.
 *
 * Applications dealing with market data often need to maintain the
 * collection of known (or valid) securities and a set of properties
 * for each one of the securities.  Please read the documentation of
 * jb::security_attributes for a motivation of the attributes attached
 * to each security.
 *
 * Often the attributes are loaded from a configuration file or
 * some kind of database.  Though these are not expected to change
 * while the application is running, an error in the preparation of
 * the configuration file, or a last minute update may force a reload
 * or change.
 *
 * Other properties of a security, such as its trading status, or
 * whether its market is open, or the last best quote are constantly
 * updated via some real-time feed.
 *
 * This class is designed to maintain such attributes that are
 * unlikely to change while the application is running.  It implements
 * copy-on-write semantics for the attributes of each symbol.  Readers
 * can fetch existing values with no locking overhead, but a write
 * will (eventually) invalidate any cached values.
 *
 * The current implementation is fairly minimal.  It was put in place
 * to support a reasonable jb::symbol class, but really needs
 * significant more work for real applications.
 */
class security_directory
    : public std::enable_shared_from_this<security_directory> {
public:
  /// Return an empty security directory
  static std::shared_ptr<security_directory> create_directory();

  /**
   * Insert or lookup a security
   *
   * This function creates a new entry for @a symbol, or if it already
   * exists it creates the index of that entry.
   *
   * @param symbol the name of the security
   * @returns the identifier for the security
   */
  security insert(std::string symbol);

  /// Insert or lookup a new security with a hint for its id
  security insert(std::string const& symbol, int id_hint);

  /// Get a static attribute
  template <typename attribute>
  typename attribute::value_type const& get_attribute(security const& sec) {
    auto entry = check_and_refresh_cached_attributes(sec);
    return entry->attributes->get<attribute>();
  }

  /// Set a static attribute
  template <typename attribute, typename U>
  void set_attribute(security const& sec, U const& v) {
    // This is always a slow operation that requires a lock ...
    std::lock_guard<std::mutex> guard(mutex_);
    // ... we want previously returned attributes to be immutable, so
    // we first make a copy of the current values ...
    int id = security_id(sec);
    auto copy = std::make_shared<security_directory_attributes>(
        *(contents_[id]->attributes));
    // ... then we set the new attribute ...
    copy->set<attribute>(v);
    // ... replace the attribute set with a new pointer
    contents_[id]->attributes = copy;
    generation_.fetch_add(1);
  }

private:
  using directory_contents =
      std::vector<std::shared_ptr<security_directory_entry>>;
  using index_by_symbol = std::map<std::string, int>;

  /// Create an empty security directory
  security_directory();

  /**
   * Return an entry for a symbol, assuming the lock is held
   *
   * @param symbol the string representation of the security (aka ticker)
   * @returns a security directory entry for the given symbol @a syn
   */
  std::shared_ptr<security_directory_entry> insert_unlocked(std::string symbol);

  /**
   * Update a security contents if necessary
   *
   * @returns the entry for the given security
   * @param sec the security to update
   */
  std::shared_ptr<security_directory_entry>
  check_and_refresh_cached_attributes(security const& sec) const;

  /// Return the id of a security (break build dependencies)
  int security_id(security const& sec) const;

private:
  /// A mutex to protect critical sections, must be mutable because it
  /// is needed in const operations
  mutable std::mutex mutex_;

  /**
   * An atomic type to sequence all updates.
   *
   * We use an atomic counter to detect if the contents have been
   * updated since the last time any data was fetch.  Most of the time
   * the data is not changed, and we can reuse the old data without
   * needing extra locks.  However, if we needed to get a new copy of
   * the data we need to acquire a mutex.
   */
  std::atomic<unsigned int> generation_;

  /// The attributes of each security, indexed by id
  directory_contents contents_;

  /// The attributes of each security, indexed by symbol (the string
  /// representation)
  index_by_symbol reverse_index_;
};

} // namespace jb

#endif // jb_security_directory_hpp
