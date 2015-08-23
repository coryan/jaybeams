#ifndef jb_severity_level_hpp
#define jb_severity_level_hpp

#include <iosfwd>
#include <string>

namespace jb {

/// Severity levels for JayBeams, based on syslog
enum class severity_level {
    trace,
    debug,
    info,
    notice,
    warning,
    error,
    critical,
    alert,
    fatal,
};

/// Streaming for severity levels
std::ostream& operator<<(std::ostream& os, severity_level const& x);
std::istream& operator>>(std::istream& is, severity_level& x);

/// Parse a severity level
void parse(severity_level& lhs, std::string const& rhs);

/// Get the name of a security level
char const* get_name(severity_level const& rhs);

/// Return the recommended with for printing security levels
int severity_level_width();

} // namespace jb

#endif // jb_severity_level
