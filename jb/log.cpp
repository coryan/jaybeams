#include <jb/log.hpp>
#include <jb/assert_throw.hpp>
#include <jb/as_hhmmss.hpp>

#include <boost/log/attributes.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iomanip>

namespace jb {
namespace log {
/// Define the default values for logging configuration
namespace defaults {

#ifndef JB_LOG_DEFAULTS_minimum_severity
#define JB_LOG_DEFAULTS_minimum_severity jb::severity_level::info
#endif // JB_LOG_DEFAULTS_minimum_severity

#ifndef JB_LOG_DEFAULTS_minimum_console_severity
#define JB_LOG_DEFAULTS_minimum_console_severity jb::severity_level::trace
#endif // JB_LOG_DEFAULTS_minimum_console_severity

#ifndef JB_LOG_DEFAULTS_enable_file_logging
#define JB_LOG_DEFAULTS_enable_file_logging false
#endif // JB_LOG_DEFAULTS_enable_file_logging

#ifndef JB_LOG_DEFAULTS_enable_console_logging
#define JB_LOG_DEFAULTS_enable_console_logging true
#endif // JB_LOG_DEFAULTS_enable_console_logging

#ifndef JB_LOG_DEFAULTS_logfile_suffix
#define JB_LOG_DEFAULTS_logfile_suffix "_%Y%m%d.%N.log"
#endif // JB_LOG_DEFAULTS_logfile_suffix

#ifndef JB_LOG_DEFAULTS_logfile_archive_directory
#define JB_LOG_DEFAULTS_logfile_archive_directory ""
#endif // JB_LOG_DEFAULTS_logfile_archive_directory

#ifndef JB_LOG_DEFAULTS_maximum_size_archived
#define JB_LOG_DEFAULTS_maximum_size_archived 128L * 1024 * 1024 * 1024
#endif // JB_LOG_DEFAULTS_maximum_size_archived

#ifndef JB_LOG_DEFAULTS_minimum_free_space
#define JB_LOG_DEFAULTS_minimum_free_space 8L * 1024 * 1024 * 1024
#endif // JB_LOG_DEFAULTS_minimum_free_space

jb::severity_level minimum_severity = JB_LOG_DEFAULTS_minimum_severity;
jb::severity_level minimum_console_severity =
    JB_LOG_DEFAULTS_minimum_console_severity;
bool enable_file_logging = JB_LOG_DEFAULTS_enable_file_logging;
bool enable_console_logging = JB_LOG_DEFAULTS_enable_console_logging;
std::string logfile_suffix = JB_LOG_DEFAULTS_logfile_suffix;
std::string logfile_archive_directory =
    JB_LOG_DEFAULTS_logfile_archive_directory;
long maximum_size_archived = JB_LOG_DEFAULTS_maximum_size_archived;
long minimum_free_space = JB_LOG_DEFAULTS_minimum_free_space;
} // namespace defaults

config::config()
    : minimum_severity(
        desc("minimum-severity").help(
            "Log messages below this severity are filtered out"),
        this, defaults::minimum_severity)
    , minimum_console_severity(
        desc("minimum-console-severity").help(
            "Log messages below this severity are filtered out in the console"),
        this, defaults::minimum_console_severity)
    , enable_console_logging(
        desc("enable-console-logging").help(
            "If set, log messages are sent to the console.  Enabled by default"),
        this, defaults::enable_console_logging)
    , enable_file_logging(
        desc("enable-file-logging").help(
            "If set, log messages are set to a log file.  Disabled by default"),
        this, defaults::enable_file_logging)
    , logfile_basename(
        desc("logfile-basename").help(
            "Define the name of the logfile,"
            " used only if enable-file-logging is true"),
        this)
    , logfile_suffix(
        desc("logfile-suffix").help(
            "Define suffix for the filename, typycally _%Y%m%d.%N.log."
            " The format characters are defined by Boost.Log"),
        this, defaults::logfile_suffix)
    , logfile_archive_directory(
        desc("logfile-archive-directory").help(
            "Define where are old (full) logfiles archived."),
        this, defaults::logfile_archive_directory)
    , maximum_size_archived(
        desc("maximum-size-archived").help(
            "Define how much space, at most, is used for saved logfiles."),
        this, defaults::maximum_size_archived)
    , minimum_free_space(
        desc("minimum-free-space").help(
            "Define how much space, at least, is kept free after cleaning up"
            " logfiles"),
        this, defaults::minimum_free_space) {
}

void config::validate() const {
  if (enable_file_logging() and logfile_basename() == "") {
    throw jb::usage("enable-file-logging is set,"
                    " you must also set logfile-basename", 1);
  }
}

BOOST_LOG_ATTRIBUTE_KEYWORD(filename, "Filename", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(lineno, "Lineno", int)
BOOST_LOG_ATTRIBUTE_KEYWORD(transaction, "Transaction", std::int64_t)
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", jb::severity_level)
BOOST_LOG_ATTRIBUTE_KEYWORD(min_severity, "MinSeverity", jb::severity_level)
BOOST_LOG_ATTRIBUTE_KEYWORD(
    current_thread_id, "ThreadID",
    boost::log::attributes::current_thread_id::value_type)
BOOST_LOG_ATTRIBUTE_KEYWORD(
    local_time, "TimeStamp",
    boost::log::attributes::local_clock::value_type)

bool filter_predicate(::boost::log::attribute_value_set const& attr) {
  // ... if we got to this point the severity is high enough, but we
  // need to silence some warnings (and also avoid lines without
  // coverage) ...
  JB_ASSERT_THROW(*attr[jb::log::severity] >= *attr[jb::log::min_severity]);
  // ... debug messages are funny, only print a few ...
  if (*attr[jb::log::severity] == jb::severity_level::debug) {
    return (*attr[jb::log::transaction] % 10000) == 0;
  }
  return true;
}

void format_common(
    boost::log::record_view const& rec,
    boost::log::formatting_ostream& strm) {
  strm << " [" << rec[jb::log::current_thread_id] << "]"
       << " [" << std::setfill(' ') << std::setw(jb::severity_level_width())
       << rec[jb::log::severity] << "]";

  if (rec[jb::log::severity] == jb::severity_level::debug) {
    strm << " tid=<" << rec[jb::log::transaction] << ">";
  }
  strm << " " << rec[boost::log::expressions::smessage]
       << " (" << rec[jb::log::filename] << ":" << rec[jb::log::lineno] << ")";
}

void format_console(
    boost::log::record_view const& rec,
    boost::log::formatting_ostream& strm) {
  auto usecs = rec[jb::log::local_time]->time_of_day().total_microseconds();

  strm << jb::as_hhmmssu(std::chrono::microseconds(usecs));
  format_common(rec, strm);
}

void format_logfile(
    boost::log::record_view const& rec,
    boost::log::formatting_ostream& strm) {
  auto date = rec[jb::log::local_time]->date();
  auto usecs = rec[jb::log::local_time]->time_of_day().total_microseconds();

  strm << std::setfill('0') << std::setw(4) << date.year()
       << '-' << std::setfill('0') << std::setw(2) << date.month().as_number()
       << '-' << std::setfill('0') << std::setw(2) << date.day()
       << ' ' << jb::as_hhmmssu(std::chrono::microseconds(usecs));
  format_common(rec, strm);
}

std::int64_t tid_;

std::int64_t tid() {
  return tid_;
}

void next_tid() {
  ++tid_;
}


void init(config const& cfg) {
  namespace expr = boost::log::expressions;
  namespace keywords = boost::log::keywords;
  auto core = boost::log::core::get();

  core->add_global_attribute(
      "Scopes", boost::log::attributes::named_scope());
  core->add_global_attribute(
      "ThreadID", boost::log::attributes::current_thread_id());
  core->add_global_attribute(
      "TimeStamp", boost::log::attributes::local_clock());
  core->add_global_attribute(
      "Transaction", boost::log::attributes::make_function(&jb::log::tid));
  core->add_global_attribute(
      "MinSeverity", boost::log::attributes::constant<jb::severity_level>(
          cfg.minimum_severity()));

  core->set_filter(&jb::log::filter_predicate);

  if (cfg.enable_console_logging()) {
    auto sink = boost::log::add_console_log(std::clog);
    sink->set_formatter(&jb::log::format_console);
    sink->set_filter( [cfg](::boost::log::attribute_value_set const& attr) {
        return *attr[jb::log::severity] >= cfg.minimum_console_severity();
      });
  }

  if (cfg.enable_file_logging()) {
    std::string filename_format = cfg.logfile_basename() + cfg.logfile_suffix();
    auto sink = boost::log::add_file_log(
        keywords::file_name = filename_format,
        keywords::time_based_rotation =
        boost::log::sinks::file::rotation_at_time_point(0, 0, 0) );
    sink->set_formatter(&jb::log::format_logfile);
    sink->locked_backend()->set_file_collector(
        boost::log::sinks::file::make_collector(
            keywords::target = cfg.logfile_archive_directory(),
            keywords::max_size = cfg.maximum_size_archived(),
            keywords::min_free_space = cfg.minimum_free_space() ) );
    sink->locked_backend()->scan_for_files();
  }
}

} // namespace log
} // namespace jb
