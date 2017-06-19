#ifndef jb_mktdata_timestamp_hpp
#define jb_mktdata_timestamp_hpp

#include <boost/endian/buffers.hpp>

namespace jb {
namespace mktdata {

/**
 * Timestamp in JayBeam messages.
 *
 * All JayBeams timestamps are expressed in nanoseconds since midnight
 * of the trading day, in the local time of the exchange.
 * TODO() we need to figure out what to do for 24x7 markets
 */
struct timestamp {
  /// The timestamp value, in nanoseconds since midnight
  boost::endian::little_uint64_buf_t nanos;
};

} // namespace mktdata
} // namespace jb

#endif // jb_mktdata_timestamp_hpp
