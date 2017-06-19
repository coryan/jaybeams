#ifndef jb_mktdata_market_id_hpp
#define jb_mktdata_market_id_hpp

#include <boost/endian/buffers.hpp>

namespace jb {
namespace mktdata {

/**
 * Market identifier in JayBeam messages.
 *
 * Sometimes it is necessary to represent a specific market in a
 * messages.  For example, for feeds that aggregate data from multiple
 * markets, or for protocols that are shared across markets.  JayBeams
 * uses a combination of a unique identifier assigned across all
 * applications, and the ISO-10383 codes (aka MIC).
 * TODO() - document how the JayBeams are assigned and maintained
 * consistently.
 */
struct market_id {
  /// The space required to send a "Market Code Identifier" (often
  /// "MIC code", as "PIN number").
  static constexpr std::size_t mic_size = 4;

  /// The JayBeams internal identifier for the market
  boost::endian::little_uint32_buf_t id;
};

} // namespace mktdata
} // namespace jb

#endif // jb_mktdata_market_id_hpp
