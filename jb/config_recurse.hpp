#ifndef jb_config_recurse_hpp
#define jb_config_recurse_hpp
/**
 * @file
 *
 * Breakout some of the helper classes from jb/config_object.hpp
 *
 * The definition of jb::config_object requires a number of helper
 * classes and functions to recurse over compound configs
 * (i.e. structs, vectors, pairs, etc.).  The code in
 * jb/config_object.hpp was getting too long to keep in a single file,
 * but these classes and the code in jb/config_attribute.hpp are also
 * deeply inter-related so they could not be simply refactored out.
 * This is a signal of poor design, and I welcome suggestions on how
 * to improve it.  However, it works, and the user interface is really
 * clean.
 */

#ifndef jb_config_attribute_hpp
#error "This file should only be included from jb/config_attribute.hpp"
#endif // !jb_config_attribute_hpp

#include <sstream>
#include <type_traits>

namespace jb {

// Forward declarations
template <typename C, typename T>
class config_attribute;

/**
 * Recursively apply functions to config_object, attributes, and
 * sequences of config objects.
 *
 * This is implemented as a struct with static functions (instead of
 * standalone functions), so jb::config_object can forward declare the
 * struct and grant 'friend' access to functions that should not be
 * exposed otherwise.
 */
struct config_recurse {
  /**
   * Override a value with the configuration contained in a YAML::Node.
   *
   * This generic implementation is used by basic types (int, string,
   * vector, etc).  We provide partial specializations for all the
   * interesting types (jb::config_attribute, jb::config_objects).
   *
   * The variadic template makes this the weakest match for resolution
   * when compared to the other partial specializations.
   */
  template <typename T, typename... Args>
  static void apply_overrides(
      T& lhs, YAML::Node const& by_name, class_overrides const& by_class,
      Args...) {
    if (not by_name or not by_name.IsDefined() or by_name.IsNull()) {
      // ... the node is not defined, nothing to override ...
      return;
    }
    lhs = std::move(by_name.as<T>());
  }

  /**
   * Partial specialization for classes derived from jb::config_object.
   */
  template <typename T>
  static typename std::enable_if<
      std::is_base_of<config_object, T>::value, void>::type
  apply_overrides(
      T& lhs, YAML::Node const& by_name, class_overrides const& by_class) {
    lhs.apply_overrides(by_name, by_class);
  }

  /**
   * Partial specialization for jb::config_attribute
   */
  template <typename C, typename T>
  static void apply_overrides(
      config_attribute<C, T>& lhs, YAML::Node const& by_name,
      class_overrides const& by_class);

  /**
   * Partial specialization for anything looking like a sequence of
   * jb::config_object.
   */
  template <typename Sequence>
  static typename std::enable_if<
      std::is_base_of<config_object, typename Sequence::value_type>::value,
      void>::type
  apply_overrides(
      Sequence& lhs, YAML::Node const& by_name,
      class_overrides const& by_class) {
    std::size_t idx = 0;
    Sequence tmp;
    for (auto i : by_name) {
      typename Sequence::value_type v;
      if (idx < lhs.size()) {
        v = std::move(lhs[idx]);
      }
      apply_overrides(v, i, by_class);
      tmp.push_back(std::move(v));
      ++idx;
    }
    for (; idx < lhs.size(); ++idx) {
      tmp.push_back(std::move(lhs[idx]));
    }
    lhs.swap(tmp);
  }

  /**
   * Implement the basic construct to create command-line options ased
   * on a config_object.
   */
  template <typename T, typename... Args>
  static void add_options(
      boost::program_options::options_description& options, T const& object,
      std::string const& prefix, config_object::attribute_descriptor const& d,
      Args...) {
    options.add_options()(
        jb::config_object::cmdline_arg_name(prefix, d.name).c_str(),
        boost::program_options::value<T>(), d.helpmsg.c_str());
  }

  /**
   * Partial specialization for classes derived from jb::config_object.
   */
  template <typename T>
  static typename std::enable_if<
      std::is_base_of<config_object, T>::value, void>::type
  add_options(
      boost::program_options::options_description& options, T const& object,
      std::string const& prefix, config_object::attribute_descriptor const& d) {
    object.add_options(
        options, jb::config_object::cmdline_arg_name(prefix, d.name), d);
  }

  /**
   * Partial specialization for configuration attribute.
   */
  template <typename C, typename T>
  static void add_options(
      boost::program_options::options_description& options,
      config_attribute<C, T> const& object, std::string const& prefix,
      config_object::attribute_descriptor const& d);

  /**
   * Partial specialization for anything looking like a sequence of
   * jb::config_object.
   */
  template <typename Sequence>
  static typename std::enable_if<
      std::is_base_of<config_object, typename Sequence::value_type>::value,
      void>::type
  add_options(
      boost::program_options::options_description& options,
      Sequence const& object, std::string const& prefix,
      config_object::attribute_descriptor const& d) {
    if (object.size() == 0) {
      typedef typename Sequence::value_type child;
      add_options(
          options, child(), jb::config_object::cmdline_arg_name(prefix, d.name),
          config_object::attribute_descriptor("0").help(d.helpmsg));
      return;
    }
    int cnt = 0;
    for (auto const& i : object) {
      std::ostringstream os;
      os << cnt;
      add_options(
          options, i, jb::config_object::cmdline_arg_name(prefix, d.name),
          config_object::attribute_descriptor(os.str()).help(d.helpmsg));
      cnt++;
    }
  }

  /**
   * Partial specialization for things that look like pairs.
   */
  template <typename U, typename V>
  static void add_options(
      boost::program_options::options_description& options,
      std::pair<U, V> const& object, std::string const& prefix,
      config_object::attribute_descriptor const& d) {
    add_options(
        options, object.first,
        jb::config_object::cmdline_arg_name(prefix, d.name),
        config_object::attribute_descriptor("first").help(
            d.helpmsg + ". Set the first field"));
    add_options(
        options, object.second,
        jb::config_object::cmdline_arg_name(prefix, d.name),
        config_object::attribute_descriptor("second").help(
            d.helpmsg + ". Set the second field"));
  }

  /**
   * Get a value from the cmdline values and apply it to the object.
   */
  template <typename T, typename... Args>
  static void apply_cmdline_values(
      T& rhs, boost::program_options::variables_map const& vm,
      std::string const& name, Args...) {
    if (vm.count(name)) {
      rhs = vm[name].as<T>();
    }
  }

  /**
   * Partial specialization for jb::config_object and derived classes.
   */
  template <typename T>
  static typename std::enable_if<
      std::is_base_of<config_object, T>::value, void>::type
  apply_cmdline_values(
      T& rhs, boost::program_options::variables_map const& vm,
      std::string const& name) {
    rhs.apply_cmdline_values(vm, name);
  }

  /**
   * Partial specialization for configuration attribute.
   */
  template <typename C, typename T>
  static void apply_cmdline_values(
      config_attribute<C, T>& rhs,
      boost::program_options::variables_map const& vm, std::string const& name);

  /**
   * Partial specialization for anything looking like a sequence of
   * jb::config_object.
   */
  template <typename Sequence>
  static typename std::enable_if<
      std::is_base_of<config_object, typename Sequence::value_type>::value,
      void>::type
  apply_cmdline_values(
      Sequence& rhs, boost::program_options::variables_map const& vm,
      std::string const& name) {
    // TODO(#6)
    if (rhs.size() == 0) {
      rhs.resize(1);
    }
    int cnt = 0;
    for (auto& i : rhs) {
      std::ostringstream os;
      os << cnt;
      apply_cmdline_values(
          i, vm, jb::config_object::cmdline_arg_name(name, os.str()));
      cnt++;
    }
  }

  /**
   * Partial specialization for std::pair<>.
   */
  template <typename U, typename V>
  static void apply_cmdline_values(
      std::pair<U, V>& rhs, boost::program_options::variables_map const& vm,
      std::string const& name) {
    apply_cmdline_values(
        rhs.first, vm, jb::config_object::cmdline_arg_name(name, "first"));
    apply_cmdline_values(
        rhs.second, vm, jb::config_object::cmdline_arg_name(name, "second"));
  }

  /// Validate an arbitrary object
  template <typename T, typename... Args>
  static void validate(T const&, Args...) {
    // do nothing, the validation for standalone attributes is usually
    // implemented by the containing config_object
  }

  /**
   * Partial specialization of validate() for jb::config_object and
   * derived classes.
   */
  template <typename T>
  static typename std::enable_if<
      std::is_base_of<config_object, T>::value, void>::type
  validate(T const& object) {
    object.validate_all();
  }

  /**
   * Partial of validate() for specialization for
   * jb::config_attribute<>.
   */
  template <typename C, typename T>
  static void validate(config_attribute<C, T> const& object);

  /**
   * Partial specialization of validate() for anything looking
   * like a sequence of jb::config_object.
   */
  template <typename Sequence>
  static typename std::enable_if<
      std::is_base_of<config_object, typename Sequence::value_type>::value,
      void>::type
  validate(Sequence const& seq) {
    for (auto const& i : seq) {
      validate(i);
    }
  }

  /**
   * Partial specialization of validate() for std::pair<>.
   */
  template <typename U, typename V>
  static void validate(std::pair<U, V> const& object) {
    validate(object.first);
    validate(object.second);
  }

  /// Convert an arbitrary type to a YAML representation
  template <typename T, typename... Args>
  static YAML::Node to_yaml(T const& x, Args...) {
    YAML::Node doc;
    doc = x;
    return doc;
  }

  /**
   * Partial specialization of to_yaml() for jb::config_object and
   * derived classes.
   */
  template <typename T>
  static typename std::enable_if<
      std::is_base_of<config_object, T>::value, YAML::Node>::type
  to_yaml(T const& object) {
    return object.to_yaml();
  }

  /**
   * Partial of to_yaml() for specialization for
   * jb::config_attribute<>.
   */
  template <typename C, typename T>
  static YAML::Node to_yaml(config_attribute<C, T> const& object);

  /**
   * Partial specialization of to_yaml() for anything looking
   * like a sequence of jb::config_object.
   */
  template <typename Sequence>
  static typename std::enable_if<
      std::is_base_of<config_object, typename Sequence::value_type>::value,
      YAML::Node>::type
  to_yaml(Sequence const& seq) {
    YAML::Node doc;
    for (auto const& i : seq) {
      doc.push_back(to_yaml(i));
    }
    return doc;
  }

  /**
   * Partial specialization of to_yaml() for std::pair<>.
   */
  template <typename U, typename V>
  static YAML::Node to_yaml(std::pair<U, V> const& object) {
    YAML::Node doc;
    doc["first"] = object.first;
    doc["second"] = object.second;
    return doc;
  }
};
} // namespace jb

#endif // jb_config_recurse_hpp
