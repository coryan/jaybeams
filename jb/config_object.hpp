#ifndef jb_config_object_hpp
#define jb_config_object_hpp

#include <jb/merge_yaml.hpp>
#include <jb/usage.hpp>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <yaml-cpp/yaml.h>
#include <iosfwd>
#include <string>
#include <vector>

namespace jb {

// Forward declare because we need to make it a 'friend'
struct config_recurse;

/**
 * Base class for all configuration objects
 *
 * A configuration object is a class derived from jb::config_object
 * and having only members of jb::config_attribute type.
 * Configuration objects can read overrides to their defaults from a
 * YAML file.
 *
 * Please @see listing:examples:configurations for some
 * examples.
 */
class config_object {
public:
  // Forward declare the type
  struct attribute_descriptor;

  /**
   * Default constructor.
   *
   * Derived classes should initialize all their member attributes to
   * their default values.
   */
  config_object();

  /**
   * Copy constructor.
   *
   * Derived classes should implement member-by-member copying using
   * the config_attribute(container*, config_attribute const&)
   * constructor.
   */
  config_object(config_object const& rhs)
      : config_object() {
  }

  /**
   * Move constructor.
   *
   * Derived classes should implement member-by-member move using the
   * config_attribute(container*, config_attribute&&) constructor.
   */
  config_object(config_object&& rhs)
      : config_object() {
  }

  /// Destructor
  virtual ~config_object() {
  }

  /**
   * Copy & swap assignment.
   */
  config_object& operator=(config_object rhs) {
    swap(rhs);
    return *this;
  }

  /**
   * Derived classes should implement member by member swap.
   */
  void swap(config_object& rhs) {
  }

  /**
   * Read the configuration file and load the overrides defined
   * therein.
   *
   * Find the correct configuration file to load using
   * jb::config_files_locations, then load the configuration file and
   * apply the overrides from that file.
   *
   * @param argc the number of command-line arguments
   * @param argv the command-line arguments
   * @param filename the basename of the file to load
   * @param environment_variable_name the name of the *_ROOT to use
   *   when searching for the configuration file.
   */
  void load_overrides(int& argc, char* argv[], std::string const& filename,
                      char const* environment_variable_name);

  /**
   * Read the configuration file and load the overrides defined
   * therein.
   *
   * Find the correct configuration file to load using
   * jb::config_files_locations, then load the configuration file and
   * apply the overrides from that file.
   *
   * @param argc the number of command-line arguments
   * @param argv the command-line arguments
   * @param filename the basename of the file to load
   */
  void load_overrides(int& argc, char* argv[], std::string const& filename);

  /**
   * Read the configuration file and load the overrides defined
   * therein.
   *
   * Find the correct configuration file to load using
   * jb::config_files_locations, then load the configuration file and
   * apply the overrides from that file.
   *
   * @param argc the number of command-line arguments
   * @param argv the command-line arguments
   * @param is the stream to load the configuration from.
   */
  void load_overrides(int& argc, char* argv[], std::istream& is);

  /**
   * Process the command line.
   */
  void process_cmdline(int& argc, char* argv[]);

  /**
   * Validate the settings
   *
   * @throws jb::usage if a problem is detected.
   */
  virtual void validate() const;

  /**
   * Print out the settings
   */
  std::ostream& to_stream(std::ostream& os) const;

  /**
   * An attribute descriptor.
   */
  struct attribute_descriptor {
    attribute_descriptor()
        : attribute_descriptor(std::string(), std::string()) {
    }
    explicit attribute_descriptor(std::string const& n)
        : attribute_descriptor(n, std::string()) {
    }
    attribute_descriptor(std::string const& n, std::string const& c)
        : name(n)
        , class_name(c)
        , helpmsg()
        , is_positional(false) {
    }

    attribute_descriptor& help(std::string const& h) {
      helpmsg = h;
      return *this;
    }

    attribute_descriptor& positional() {
      is_positional = true;
      return *this;
    }

    std::string name;
    std::string class_name;
    std::string helpmsg;
    bool is_positional;
  };

  /**
   * Define the interface to manipulate and access configuration
   * attributes embedded in a config_object.
   */
  class attribute_base {
  protected:
    /// Constructor
    attribute_base(attribute_descriptor const& d, config_object* container);

    template <typename container_type>
    attribute_base(container_type*)
        : descriptor_() {
    }

  public:
    /// Destructor
    virtual ~attribute_base() = 0;

    /// Apply any overrides set in the YAML document
    virtual void apply_overrides(YAML::Node const& by_name,
                                 class_overrides const& by_class) = 0;

    /// Apply the necessary command-line options to the descriptors.
    virtual void
    add_options(boost::program_options::options_description& options,
                std::string const& prefix,
                attribute_descriptor const& d) const = 0;

    /// Apply any overrides set in the command-line flags
    virtual void
    apply_cmdline_values(boost::program_options::variables_map const& vm,
                         std::string const& name) = 0;

    /// Validate the attribute, mostly a no-op except for embedded
    /// config_objects
    virtual void validate() const = 0;

    /// Convert to a YAML Node, useful to dump the configuration
    virtual YAML::Node to_yaml() const = 0;

    attribute_descriptor const& descriptor() const {
      return descriptor_;
    }

    std::string const& name() const {
      return descriptor_.name;
    }
    std::string const& class_name() const {
      return descriptor_.class_name;
    }
    std::string const& help() const {
      return descriptor_.helpmsg;
    }
    bool positional() const {
      return descriptor_.is_positional;
    }

  private:
    attribute_descriptor const descriptor_;
  };

protected:
  /// Convenience function to create attribute descriptors with less typing.
  static attribute_descriptor desc(std::string const& name) {
    return attribute_descriptor(name);
  }

  /// Convenience function to create attribute descriptors with less typing.
  static attribute_descriptor desc(std::string const& name,
                                   std::string const& class_name) {
    return attribute_descriptor(name, class_name);
  }

private:
  /**
   * Apply the overrides contained in the YAML document, compute the
   * initial by_class overrides.
   *
   * @param by_name a YAML document describing what values (and
   *   classes) to override.
   */
  void apply_overrides(YAML::Node const& by_name);

  /**
   * Apply the overrides contained in the YAML document.
   *
   * @param by_name a YAML document describing what values (and
   *   classes) to override.
   * @param by_class the set of per-class overrides to apply
   */
  void apply_overrides(YAML::Node const& by_name,
                       class_overrides const& by_class);

  /**
   * Compute the full name of a command-line argument, given its
   * prefix and short name.
   *
   * @param prefix the prefix for the argument
   * @param name the name of the argument
   */
  static std::string cmdline_arg_name(std::string const& prefix,
                                      std::string const& name);

  friend class generic_config_attribute;
  void auto_register(attribute_base* a);

  /// Run validate() on each attribute contained by this config_object.
  void validate_attributes() const;

  /// Validate both the config_object and each attribute.
  friend struct config_recurse;
  void validate_all() const;

  /**
   * Add the attributes of this config_object as command-line options.
   */
  void add_options(boost::program_options::options_description& options,
                   std::string const& prefix,
                   attribute_descriptor const& d) const;

  /**
   * Apply the values from the cmdline to this configuration object.
   */
  void apply_cmdline_values(boost::program_options::variables_map const& vm,
                            std::string const& prefix);

  /// Print out the configuration settings in YAML format
  YAML::Node to_yaml() const;

private:
  /// The list of attributes.
  std::vector<attribute_base*> attributes_;
};

inline std::ostream& operator<<(std::ostream& os, config_object const& x) {
  return x.to_stream(os);
}

} // namespace jb

#define config_object_constructors(NAME)                                       \
  NAME(NAME&& rhs)                                                             \
      : NAME() {                                                               \
    operator=(std::move(rhs));                                                 \
  }                                                                            \
  NAME(NAME const& rhs)                                                        \
      : NAME() {                                                               \
    operator=(rhs);                                                            \
  }                                                                            \
  NAME& operator=(NAME const& rhs) = default;                                  \
  NAME& operator=(NAME&& rhs) = default

#include <jb/config_recurse.hpp>
#include <jb/config_attribute.hpp>

#endif // jb_config_object_hpp
