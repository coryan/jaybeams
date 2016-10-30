#ifndef jb_itch5_mpid_field_hpp
#define jb_itch5_mpid_field_hpp

#include <jb/itch5/short_string_field.hpp>

namespace jb {
namespace itch5 {

/**
 * Define the type used to represent the 'MPID' field in many
 * messages.
 *
 * Some ITCH-5.0 messages need to include information about the Market
 * Participant that the message refers to.  For example, members of
 * the NASDAQ exchange may chose to identify their orders instead of
 * trading anonymously.  In the NASDAQ exchange each participant
 * receives a 4-letter (all uppercase) identifier.
 */
typedef jb::itch5::short_string_field<4> mpid_t;

} // namespace itch5
} // namespace jb

#endif // jb_itch5_mpid_field_hpp
