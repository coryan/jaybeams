#ifndef jb_mktdata_inside_levels_update_hpp
#define jb_mktdata_inside_levels_update_hpp

#include <jb/mktdata/detail/levels_name.hpp>
#include <jb/mktdata/feed_id.hpp>
#include <jb/mktdata/market_id.hpp>
#include <jb/mktdata/security_id.hpp>
#include <jb/mktdata/timestamp.hpp>

namespace jb {
namespace mktdata {

/**
 * A message representing the top N levels of a market.
 *
 * Many applications need more information than is provided in a Level
 * I (aka top of book) feed, but are easier to implement and support
 * if they do not have the complexity of a Level II or Level III feed,
 * where one needs to deal with stateful message streams.
 *
 * This feed is a compromise, it is stateless, so simple to process,
 * but it provides only the top N levels (typically 1, 4 or 8), so
 * less information rich than a Level II or Level III feed.  The
 * assumption is that most of the information is in the top N levels
 * anyway, so the loss is minimal.
 *
 * It also tradesoff simplicity for heavier message payloads, which
 * might be a problem in some applications.
 *
 * @tparam N the number of levels to support.
 */
template <std::size_t N>
struct inside_levels_update {
  static constexpr std::uint16_t mtype =
      u'I' << 8 | detail::levels_name<N>::name;

  /// The message type, each message in JayBeams receives a unique
  /// identifier
  boost::endian::little_uint16_buf_t message_type;

  /**
   * The message size.
   *
   * While the size of the message is implicit in the C++ structure
   * used to represent them, we include the message size
   */
  boost::endian::little_uint16_buf_t message_size;

  /// The sequence number created by the feed handler
  boost::endian::little_uint32_buf_t sequence_number;

  /// The market this data refers to
  market_id market;

  /// The name of the feed handler used to parse and generate this
  /// data
  feed_id feed;

  /// The feedhandler (the software system that processes the feed and
  /// generated this message), timestamps the message just before
  /// sending it out.
  timestamp feedhandler_ts;

  /// The source of the data within that feed, some feeds arbitrage
  /// between multiple sources for the same data.
  feed_id source;

  /// Typically exchange feeds provide a timestamp (with feed-specific
  /// semantics) for the event in the exchange that generated a
  /// message.  This field contains that timestamp.
  timestamp exchange_ts;

  /// Typically each feed provides a timestamp (with feed-specific
  /// semantics) for the message, this may be different from the
  /// exchange timestamp.
  timestamp feed_ts;

  /// The id of the security
  security_id security;

  /// The bid quantities, in shares, can be 0 if the level does not
  /// exist or is not provided by the exchange.  The bid levels are in
  /// descending order of price.
  boost::endian::little_uint32_buf_t bid_qty[N];

  /**
   * Bid prices, in descending order.
   *
   * For the US markets, JayBeams uses prices in multiples of $0.0001.
   * Since the US markets do not allow quotes in smaller intervals
   * this is has no loss of accuracy.  The maximum quote value in the
   * US markets is $200,000, so at the prescribed granularity this
   * fits in a 32-bit integer.
   * TODO() - we need to define how this works in other markets,
   * it is likely that this would require 64-bit integers for Japan
   * for example.
   */
  boost::endian::little_uint32_buf_t bid_px[N];

  /// Offer quantities, in ascending order of prices
  boost::endian::little_uint32_buf_t offer_qty[N];

  /**
   * Offer prices, in ascending order.
   *
   * For the US markets, JayBeams uses prices in multiples of $0.0001.
   * Since the US markets do not allow quotes in smaller intervals
   * this is has no loss of accuracy.  The maximum quote value in the
   * US markets is $200,000, so at the prescribed granularity this
   * fits in a 32-bit integer.
   * TODO() - we need to define how this works in other markets,
   * it is likely that this would require 64-bit integers for Japan
   * for example.
   * TODO() - likely this should be a message, like jb::mktdata::timestamp.
   */
  boost::endian::little_uint32_buf_t offer_px[N];

  //@{
  /**
   * Annotations
   *
   * These annotations are optional, they may not appear in a
   * production feed to minimize message size and processing time.  On
   * a development instance these will be populated with the human
   * readable representation of several fields.
   * The receiver can detect whether these fields are present using
   * the message_size field at the beginning.
   */
  struct annotations_type {
    /// The ISO-10383 market code
    boost::endian::little_uint8_buf_t mic[market_id::mic_size];
    /// The name of the feed
    boost::endian::little_uint8_buf_t feed_name[feed_id::feed_name_size];
    /// The name of the data source
    boost::endian::little_uint8_buf_t source_name[feed_id::feed_name_size];
    /// The JayBeams normalized ticker for the security
    boost::endian::little_uint8_buf_t
        security_normalized[security_id::normalized_size];
    /// The ticker as it appears in the feed
    boost::endian::little_uint8_buf_t security_feed[security_id::feed_size];
  };
  /// The annotations field
  annotations_type annotations;
  //@}
};

} // namespace mktdata
} // namespace jb

#endif // jb_mktdata_inside_levels_update_hpp
