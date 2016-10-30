#ifndef jb_itch5_protocol_constants_hpp
#define jb_itch5_protocol_constants_hpp

namespace jb {
namespace itch5 {
/**
 * ITCH-5.x protocol constants
 */
namespace protocol {

/**
 * The total size of the ITCH-5.x header.
 */
constexpr std::size_t header_size = (
    /* message type */ 1 +
    /* stock locate */ 2 +
    /* tracking number */ 2 +
    /* timestamp */ 6);

/**
 * The maximum size for a ITCH-5.x message (the length is a 16-bit
 * integer)
 */
constexpr std::size_t max_message_size = (1 << 16) - 1;

} // namespace protocol
} // namespace itch5
} // namespace jb

#endif // jb_itch5_protocol_constants_hpp
