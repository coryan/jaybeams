#ifndef jb_ehs_base_types_hpp
#define jb_ehs_base_types_hpp

#include <beast/http.hpp>

namespace jb {
/**
 * Contains types and classes to implemented Embedded HTTP Servers.
 *
 * JayBeams long running server applications, such as feed handlers or
 * data quality monitoring applications are controlled and monitored
 * using an Embedded HTTP Server.
 */
namespace ehs {

/// The request type used for JayBeams Embedded HTTP Servers.
using request_type = beast::http::request<beast::http::string_body>;

/// The response type used for JayBeams Embedded HTTP Servers.
using response_type = beast::http::response<beast::http::string_body>;

} // namespace ehs
} // namespace jb

#endif // jb_ehs_base_types_hpp
