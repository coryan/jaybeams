#ifndef jb_mktdata_security_id_hpp
#define jb_mktdata_security_id_hpp

#include <boost/endian/buffers.hpp>

namespace jb {
namespace mktdata {

/**
 * Security identifiers in JayBeam messages.
 *
 * Market feeds often use a short string (or sometimes a number,
 * represented as a decimal string) to represent a security.
 * Using strings to represent the most common identifier in market
 * data applications is wasteful, JayBeams assigns a unique number to
 * each security, and propagates the number through the system.
 *
 * Applications that need to print the identifier in human readable
 * form, or send it outside the system (say for clearing, or order
 * placement) need to lookup the identifier in a table.
 * TODO() - implement the table using etcd and updates from market
 * feeds.
 * TODO() - implement the table using a well known list for testing
 * TODO() - implement the table using flat files for testing
 */
struct security_id {
  /// The JayBeams internal identifier for the security
  boost::endian::little_uint32_buf_t id;

  /**
   * The maximum size for normalized security tickers in JayBeams.
   *
   * We do not have an authoritative source for the maximum ticker
   * name globally.  ISO-6166 (ISIN) only require 12 characters.
   * In my experience no US equity market requires more than
   * 8 characters.  The US option markets require 21
   * characters for a security:
   *   https://en.wikipedia.org/wiki/Option_symbol
   * TODO() - check global markets, particularly Japan and the UK.
   * Japan as I recall uses a long number, and UK uses SEDOLs.
   */
  static constexpr std::size_t normalized_size = 24;

  /**
   * The maximum expected size for security tickers in JayBeams.
   *
   * See the comment for the normalized ticker sizes.
   */
  static constexpr std::size_t feed_size = normalized_size;
};

} // namespace mktdata
} // namespace jb

#endif // jb_mktdata_security_id_hpp
