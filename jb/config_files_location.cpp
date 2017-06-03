#include "jb/config_files_location.hpp"

#include <boost/filesystem.hpp>
#include <stdexcept>

namespace fs = boost::filesystem;

jb::config_files_locations_base::config_files_locations_base(
    boost::filesystem::path const& argv0,
    std::function<char const*(char const*)> getenv,
    char const* program_root_variable)
    : search_path_() {
  // Discover the name of the configuration directory for the platform
  fs::path sysconfdir = jb::sysconfdir();
  fs::path bindir = jb::bindir();
  fs::path sysconf_leaf = sysconfdir.filename();
  fs::path bin_leaf = bindir.filename();

  // If there is a *_ROOT environment variable defined, use that ...
  if (program_root_variable != nullptr) {
    char const* program_root = getenv(program_root_variable);
    if (program_root != nullptr) {
      fs::path dir = fs::path(program_root) / sysconf_leaf;
      search_path_.push_back(dir);
    }
  }

  // ... otherwise, if ${SYSTEM}_ROOT is defined, use that ...
  char const* system_root = getenv("JAYBEAMS_ROOT");
  if (system_root != nullptr) {
    fs::path dir = fs::path(system_root) / sysconf_leaf;
    search_path_.push_back(dir);
  }

  // ... otherwise, search in the ${SYSTEM}_SYSCONFDIR defined at
  // compile-time ...
  search_path_.push_back(sysconfdir);

  // ... otherwise, discover where the program is running and guess
  // where the data directory might be ...
  fs::path program(argv0);
  if (program.parent_path().filename() == bin_leaf) {
    fs::path dir = program.parent_path().parent_path() / sysconf_leaf;
    search_path_.push_back(dir);
  } else if (program.parent_path() != "") {
    search_path_.push_back(program.parent_path());
  }
}

jb::config_files_locations_base::config_files_locations_base(
    boost::filesystem::path const& argv0,
    std::function<char const*(char const*)> getenv)
    : config_files_locations_base(argv0, getenv, nullptr) {
}

char const* jb::default_getenv::operator()(char const* name) {
  return std::getenv(name);
}

bool jb::default_validator::operator()(fs::path const& dir) {
  return fs::exists(dir);
}

// This is an ugly mess of #ifdefs, but better here than sprinkled
// around in the code.

#ifndef JB_DEFAULT_SYSCONFDIR
#define JB_DEFAULT_SYSCONFDIR "/etc"
#endif // JB_DEFAULT_SYSCONFDIR
#ifndef JB_DEFAULT_BINDIR
#define JB_DEFAULT_BINDIR "/usr/bin"
#endif // JB_DEFAULT_BINDIR

char const* jb::sysconfdir() {
#if defined(JB_SYSCONFDIR)
  return JB_SYSCONFDIR;
#else
  return JB_DEFAULT_SYSCONFDIR;
#endif // JB_SYSCONFDIR
}

char const* jb::bindir() {
#if defined(JB_BINDIR)
  return JB_BINDIR;
#else
  return JB_DEFAULT_BINDIR;
#endif // JB_BINDIR
}
