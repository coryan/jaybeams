#ifndef jb_config_attribute_hpp
#define jb_config_attribute_hpp

#ifndef jb_config_object_hpp
#error "This file should only be included from jb/config_object.hpp"
#endif // jb_config_object_hpp

#include <utility>

namespace jb {

/**
 * Helper class to easily define configuration attributes.
 *
 * The Jaybeams configuration framework requires all configuration
 * objects to define accessors and modifiers for each attribute that
 * follow this pattern:
 *
 * @code
 * class Config {
 * public:
 *  Config(); // default constructor sets to compile-time defaults.
 *
 *  int attribute() const; // return current attribute value
 *  Config& attribute(int x); // set current value, chainable
 * };
 *
 * Config cfg;
 * cfg.attribute(1).another(2);
 *
 * @endcode
 *
 * This is a helper class to create such attributes easily.
 *
 * @tparam C the type of the object that contains the attribute
 * @tparam T the type of the object contained in this attribute
 */
template<typename C, typename T>
class config_attribute : public config_object::attribute_base {
 public:
  typedef C container_type;
  typedef T value_type;

  /// Swap values, containers are not changed
  void swap(config_attribute<container_type,value_type>& rhs) {
    std::swap(value_, rhs.value_);
  }

  /// Accessor
  value_type const& operator()() const {
    return value_;
  }

  /// Modify value
  container_type& operator()(value_type const& x) {
    value_ = x;
    return *container_;
  }

  /// Modify value with move semantics
  container_type& operator()(value_type&& x) {
    value_ = x;
    return *container_;
  }

 private:
  //@{
  /**
   * @name Constructors
   *
   * Notice that only the container class can create these objects.
   */
  friend container_type;
  explicit config_attribute(container_type* container)
      : config_object::attribute_base(container)
      , container_(container)
      , value_() {
  }
  config_attribute(
      config_object::attribute_descriptor const& d, container_type* container)
      : config_object::attribute_base(d, container)
      , container_(container)
      , value_() {
  }

  config_attribute(container_type* container, value_type const& t)
      : config_object::attribute_base(container)
      , container_(container)
      , value_(t) {
  }

  config_attribute(
      config_object::attribute_descriptor const& d, container_type* container,
      value_type const& t)
      : config_object::attribute_base(d, container)
      , container_(container)
      , value_(t) {
  }

  config_attribute(container_type* container, value_type&& t)
      : config_object::attribute_base(container)
      , container_(container)
      , value_(std::move(t)) {
  }

  config_attribute(
      config_object::attribute_descriptor const& d, container_type* container,
      value_type&& t)
      : config_object::attribute_base(d, container)
      , container_(container)
      , value_(std::move(t)) {
  }

  template<typename... Args>
  config_attribute(container_type* container, Args&&... args)
      : config_object::attribute_base(container)
      , container_(container)
      , value_(std::forward<Args>(args)...) {
  }

  template<typename... Args>
  config_attribute(
      config_object::attribute_descriptor const& d,
      container_type* container, Args&&... args)
      : config_object::attribute_base(d, container)
      , container_(container)
      , value_(std::forward<Args>(args)...) {
  }

  config_attribute(
      container_type* container,
      config_attribute<container_type,value_type>&& rhs)
      : config_object::attribute_base(container)
      , container_(container)
      , value_(std::move(rhs.value_)) {
  }

  config_attribute(
      container_type* container,
      config_attribute<container_type,value_type> const& rhs)
      : config_object::attribute_base(container)
      , container_(container)
      , value_(rhs.value_) {
  }

  config_attribute(config_attribute<container_type,value_type>&&) = delete;
  config_attribute(config_attribute<container_type,value_type> const&) = delete;

  config_attribute& operator=(
      config_attribute<container_type,value_type>&& rhs) {
    value_ = std::move(rhs.value_);
    return *this;
  }
  config_attribute& operator=(
      config_attribute<container_type,value_type> const& rhs) {
    value_ = rhs.value_;
    return *this;
  }
  //@}

  friend struct config_recurse;
  //@{
  /**
   * @name Configuration object recursion functions.
   */
  /// Call the right version of jb::apply_overrides for the contained value.
  void apply_overrides(
      YAML::Node const& by_name,
      class_overrides const& by_class) override {
    jb::config_recurse::apply_overrides(value_, by_name, by_class);
  }

  /// Call the right version of jb::add_options for the contained value.
  void add_options(
      boost::program_options::options_description& options,
      std::string const& prefix,
      config_object::attribute_descriptor const& d) const override {
    jb::config_recurse::add_options(options, value_, prefix, d);
  }

  /// Call the right version of jb::apply_cmdline_values.
  void apply_cmdline_values(
      boost::program_options::variables_map const& vm,
      std::string const& name) override {
    jb::config_recurse::apply_cmdline_values(value_, vm, name);
  }

  /// Call the right version of jb::validate.
  void validate() const override {
    jb::config_recurse::validate(value_);
  }

  /// Print out the configuration settings in YAML format
  YAML::Node to_yaml() const override {
    return jb::config_recurse::to_yaml(value_);
  }
  //@}

 private:
  container_type* const container_;
  value_type value_;
};

template<typename C, typename T>
void config_recurse::apply_overrides(
    config_attribute<C,T>& lhs, YAML::Node const& by_name,
    class_overrides const& by_class) {
  lhs.apply_overrides(by_name, by_class);
}

template<typename C, typename T>
void config_recurse::add_options(
    boost::program_options::options_description& options,
    config_attribute<C,T> const& object, std::string const& prefix,
    config_object::attribute_descriptor const& d) {
  object.add_options(options, prefix, d);
}

template<typename C, typename T>
void config_recurse::apply_cmdline_values(
    config_attribute<C,T>& rhs, boost::program_options::variables_map const& vm,
    std::string const& name) {
  rhs.apply_cmdline_values(vm, name);
}

template<typename C, typename T> 
void config_recurse::validate(config_attribute<C,T> const& rhs) {
  rhs.validate();
}

template<typename C, typename T> 
YAML::Node config_recurse::to_yaml(config_attribute<C,T> const& rhs) {
  return rhs.to_yaml();
}

} // namespace jb

#endif // jb_config_attribute_hpp
