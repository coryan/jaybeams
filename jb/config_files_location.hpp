#ifndef jb_config_files_location_hpp
#define jb_config_files_location_hpp

#include <boost/filesystem/path.hpp>
#include <cstdlib>
#include <string>
#include <vector>

namespace jb {

/**
 * Define the functor to read configuration variables from the
 * environment.  This is mostly to provide a dependency injection
 * point for tests.
 */
struct default_getenv {
  char const* operator()(char const* name);
};

/**
 * Define the functor to validate if paths are readable.  This is
 * mostly to provide a dependency injection point for tests.
 */
struct default_validator {
  bool operator()(boost::filesystem::path const& path);
};

/**
 * Define the configuration file search algorithm for JayBeams.
 *
 * Please @see config_files_location_base, this class just refactors
 * the implementation to a non-template class.
 */
class config_files_locations_base {
public:
  std::vector<boost::filesystem::path> const& search_path() const {
    return search_path_;
  }

protected:
  /**
   * Constructor.
   *
   * @param argv0 the path for the current program (typically
   *   argv[0])
   * @param program_root_variable the name of the *_ROOT environment variable
   *   used by this program, it can be null if the program does not
   *   provide any.
   * @param getenv a function to fetch the values of environment
   *   variables.  Just used for dependency injection.
   */
  config_files_locations_base(
      boost::filesystem::path const& argv0,
      std::function<char const*(char const*)> getenv,
      char const* program_root_variable);

  /**
   * Constructor.
   *
   * @param argv0 the path for the current program (typically
   *   argv[0])
   * @param getenv a function to fetch the values of environment
   *   variables.  Just used for dependency injection.
   */
  config_files_locations_base(
      boost::filesystem::path const& argv0,
      std::function<char const*(char const*)> getenv);

private:
  std::vector<boost::filesystem::path> search_path_;
};

/**
 * Compute the directories where a configuration file can be found.
 *
 * JayBeams configuration files can be located in multiple places:
 * # Each program defines its own '*_ROOT' environment variable, the
 *   files are searched there if the variable is defined, otherwise
 * # the files are found in the generic 'JAYBEAMS_ROOT' location, if
 *   that environment variable is defined, otherwise
 * # the files are found in the installation directory, if the
 *   directory exists, otherwise,
 * # the files are found relative to the program path name.
 *
 * This class implements the algorithms to (a) compute the valid list
 * of directories for a given program, and (b) select which
 * configuration file to load.
 *
 * We use some template parameters for dependency injection, it is
 * hard to test the interaction with the file system (and the
 * environment variables) without them.
 *
 * @tparam getenv_functor the functor used to fetch the value of
 *   environment variables.  This is used for dependency injection
 *   during testing.
 * @tparam validator_functor the functor used to validate if a given
 *   path is readable.  This is used for dependency injection during
 *   testing.
 */
template <
    typename getenv_functor = default_getenv,
    typename validator_functor = default_validator>
class config_files_locations : public config_files_locations_base {
public:
  /**
   * Constructor.
   *
   * Build the search path given the program path and the name of its
   * preferred environment variable.
   *
   * @param argv0 the program path, typically argv[0] from main()
   * @param program_root_variable the name of the preferred
   * environment variable for this program, i.e., its *_ROOT variable.
   * @param getenv the functor object to use
   */
  config_files_locations(
      boost::filesystem::path const& argv0, char const* program_root_variable,
      getenv_functor getenv)
      : config_files_locations_base(argv0, getenv, program_root_variable) {
  }

  /**
   * Constructor.
   *
   * Build the search path given the program path.  Used in programs
   * that do not have a preferred *_ROOT environment variable.
   *
   * @param argv0 the program path, typically argv[0] from main()
   * @param getenv the functor object to use
   */
  config_files_locations(
      boost::filesystem::path const& argv0, getenv_functor getenv)
      : config_files_locations_base(argv0, getenv) {
  }

  ///@{
  /**
   * Convenience constructors.
   *
   * Apply common conversions and create functors as needed.
   */
  config_files_locations(
      boost::filesystem::path const& argv0, char const* program_root_variable)
      : config_files_locations(argv0, program_root_variable, getenv_functor()) {
  }
  explicit config_files_locations(boost::filesystem::path const& argv0)
      : config_files_locations(argv0, getenv_functor()) {
  }
  config_files_locations(
      char const* argv0, char const* program_root_variable,
      getenv_functor getenv)
      : config_files_locations(
            boost::filesystem::path(argv0), program_root_variable, getenv) {
  }
  config_files_locations(char const* argv0, char const* program_root_variable)
      : config_files_locations(
            boost::filesystem::path(argv0), program_root_variable,
            getenv_functor()) {
  }
  config_files_locations(char const* argv0, getenv_functor getenv)
      : config_files_locations_base(boost::filesystem::path(argv0), getenv) {
  }
  explicit config_files_locations(char const* argv0)
      : config_files_locations_base(
            boost::filesystem::path(argv0), getenv_functor()) {
  }
  //@}

  /**
   * Find a configuration file in the computed search path.
   *
   * @param filename the basename of the configuration file.
   * @param validator the functor to use for validation
   *
   * @returns the path to load
   * @throws std::runtime_error if no suitable file was found.
   */
  boost::filesystem::path find_configuration_file(
      std::string const& filename, validator_functor validator) const {
    for (auto const& path : search_path()) {
      auto full = path / filename;
      if (validator(full)) {
        return full;
      }
    }
    std::string msg("Cannot find file in search path: ");
    msg += filename;
    throw std::runtime_error(msg);
  }

  /**
   * Find a configuration file in the computed search path.
   *
   * @param filename the basename of the configuration file.
   *
   * @returns the path to load
   * @throws std::runtime_error if no suitable file was found.
   */
  boost::filesystem::path
  find_configuration_file(std::string const& filename) const {
    return find_configuration_file(filename, validator_functor());
  }
};

/**
 * Return the system configuration directory.
 *
 * Typically this is something like /etc or /usr/local/etc but it can
 * change depending on how JayBeams was configured and the target
 * platform.
 */
char const* sysconfdir();

/**
 * Return the binary installation directory directory.
 *
 * Typically this is something like /usr/bin or /usr/local/bin but it
 * can change depending on how JayBeams was configured and the
 * target platform.
 */
char const* bindir();

} // namespace jb

#endif // jb_config_files_location_hpp
