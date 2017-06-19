#ifndef jb_mktdata_feed_id_hpp
#define jb_mktdata_feed_id_hpp

#include <boost/endian/buffers.hpp>

namespace jb {
namespace mktdata {

/**
 * Identifiers for market feeds.
 *
 * Each feed in JayBeams receives a unique identifier, for example,
 * the Nasdaq OMX feed over ITCH-5.x will have a different name than
 * the PSX feed over ITCH-5.x.  This is useful for applications that
 * want to detect bad feeds and name them, or simply for
 * debugability.
 */
struct feed_id {
  /// The JayBeams internal identifier for the feed
  boost::endian::little_uint32_buf_t id;

  /// The size for a feed name, in bytes
  static constexpr std::size_t feed_name_size = 16;
};

} // namespace mktdata
} // namespace jb

#endif // jb_mktdata_feed_id_hpp
