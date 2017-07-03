#ifndef jb_log_hpp
#define jb_log_hpp

#include <jb/config_object.hpp>
#include <jb/convert_severity_level.hpp>

#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

#ifndef JB_MIN_LOG_LEVEL
#define JB_MIN_LOG_LEVEL debug
#endif // JB_MIN_LOG_LEVEL

namespace jb {

/// Statically check if the severity level should even be compiled in.
constexpr bool severity_static_predicate(jb::severity_level lvl) {
  return lvl >= jb::severity_level::JB_MIN_LOG_LEVEL;
}

/// Logging functions and objects for JayBeams
namespace log {

/**
 * Configuration object for the logging functions.
 */
class config : public jb::config_object {
public:
  config();
  config_object_constructors(config);

  jb::config_attribute<config, jb::severity_level> minimum_severity;
  jb::config_attribute<config, jb::severity_level> minimum_console_severity;
  jb::config_attribute<config, bool> enable_console_logging;
  jb::config_attribute<config, bool> enable_file_logging;
  jb::config_attribute<config, std::string> logfile_basename;
  jb::config_attribute<config, std::string> logfile_suffix;
  jb::config_attribute<config, std::string> logfile_archive_directory;
  jb::config_attribute<config, long> maximum_size_archived;
  jb::config_attribute<config, long> minimum_free_space;

  void validate() const override;
};

/// Initialize the logging functions using the configuration provided
void init(config const& cfg = config());

/// Define the global logger for JayBeams
BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(
    logger, boost::log::sources::severity_logger_mt<severity_level>)

void next_tid();

} // namespace log

} // namespace jb

#define JB_LOG_I(logger, lvl, rec_var)                                         \
  if (not::jb::severity_static_predicate(lvl)) {                               \
  } else                                                                       \
    for (::boost::log::record rec_var =                                        \
             (logger).open_record(boost::log::keywords::severity = lvl);       \
         !!rec_var;)                                                           \
    ::boost::log::aux::make_record_pump((logger), rec_var).stream()            \
        << boost::log::add_value("Filename", __FILE__)                         \
        << boost::log::add_value("Lineno", static_cast<int>(__LINE__))

#define JB_LOG(lvl)                                                            \
  JB_LOG_I(                                                                    \
      ::jb::log::logger::get(), jb::severity_level::lvl,                       \
      BOOST_LOG_UNIQUE_IDENTIFIER_NAME(jb_log_record_))

#define JB_LOG_VAR(lvl)                                                        \
  JB_LOG_I(                                                                    \
      ::jb::log::logger::get(), lvl,                                           \
      BOOST_LOG_UNIQUE_IDENTIFIER_NAME(jb_log_record_))

#endif // jb_log_hpp
