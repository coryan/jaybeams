#ifndef jb_itch5_mold_udp_protocol_constants_hpp
#define jb_itch5_mold_udp_protocol_constants_hpp

namespace jb {
namespace itch5 {
/**
 * MoldUDP64 protocol constants
 */
namespace mold_udp_protocol {

/**
 * The size of the session_id field, this is the first field of the
 * header, at offset 0.
 */
constexpr std::size_t session_id_size = 10;

/**
 * The location of the sequence number field within the header.
 */
constexpr std::size_t sequence_number_offset = session_id_size;

/**
 * The location of the block count field within the header.
 */
constexpr std::size_t block_count_offset = sequence_number_offset + 8;

/**
 * The total size of the MoldUDP64 header.
 */
constexpr std::size_t header_size = block_count_offset + 2;

} // namespace mold_udp_protocol
} // namespace itch5
} // namespace jb

#endif // jb_itch5_mold_udp_protocol_constants_hpp
