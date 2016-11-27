#ifndef jb_security_attributes_hpp
#define jb_security_attributes_hpp

#include <boost/any.hpp>
#include <atomic>
#include <type_traits>
#include <vector>

namespace jb {

/**
 * Define facilities to maintain a set of attributes for a security.
 *
 * Applications dealing with market data often have to maintain
 * attributes for each security they process.
 *
 * For example, the application may need to know what is the "lot
 * size" (the typical trading unit), what is the primary market for a
 * security, or what quoting rules apply to the security.  Generally,
 * the attributes of a security can be grouped into static attributes
 * vs. dynamic ones.  The static attributes are not expected to change
 * during a trading day (for example the lot size), while other
 * attributes may be expected to change, but rarely (such as whether
 * the security is halted).
 *
 * An additional challenge of keeping these attributes is that they
 * often change by application.  Some code may define attributes for
 * purely technical reasons: say a strategy class that defines how the
 * security is quoted.  Such an attribute is valid only in the context
 * of an specific program.
 *
 * In other words, the security attributes must be dynamic, i.e., we
 * canot use a struct or class to represent all the attributes for a
 * security.  We also want relatively fast access, so any lookup by
 * name or similar searches would too slow to be practical.  And
 * finally, because this is C++, we want interfaces that are
 * reasonably type-safe.
 *
 * Finally, because applications may want to group attributes in
 * different sets we make the security_attributes class parametric on
 * yet another tag class.  That way the application can use different
 * groupings, for example:
 *
 * struct dynamic_attributes_tag {}
 * typedef security_attributes<dynamic_attributes_tag> dynamic_group;
 * struct loaded_attributes_tag {}
 * typedef security_attributes<loaded_attributes_tag> loaded_group;
 *
 * To balance these contraints we define a unique type for each
 * attribute in each group.  The type serves only as a tag to identify
 * the attribute and the C++ type stored in it.  For example one might
 * use:
 *
 *    struct lot_size_tag {};
 *    typedef loaded_group::attribute<lot_size_tag, int> lot_size_attribute;
 *
 * The application can use the tag as an identifier:
 *
 *    loaded_group group = ...;
 *    attributes.set<lot_size_attribute>(100);
 *    assert(attributes.get<log_size_attribute>() == 100);
 *
 * @tparam tag a type used to create different groups of attributes
 */
template <typename tag>
class security_attributes {
public:
  /**
   * Define an attribute in this group.
   *
   * Applications wanting to add attributes to a given group must
   * define them using this template class.  Please see the
   * documentation of jb::security_attributes for an example.
   */
  template <typename attribute_tag, typename value_t>
  struct attribute {
    //@{
    /// @name type traits

    /// The type of the values stored in this attribute
    typedef value_t value_type;
    /// The tag for this attribute
    typedef attribute_tag tag_type;
    /// The group this attribute can be used with
    typedef security_attributes group_type;
    //@}

    /**
     * The (internal) identifier for this attribute.
     *
     * This variable is initialized during program startup (before
     * main), and should not be modified by the application.
     */
    static int const id;
  };

  /**
   * Set the value of an attribute
   *
   * @tparam attribute_type the tag identifying the attribute to set,
   * must be an instantiation of the @a attribute class
   * @param t the value of the attribute
   * @returns the value of the @a attribute_type attribute
   * @throws boost::bad_any_cast if there is a type mismatch
   */
  template <typename attribute_type, typename U>
  void set(U const& t) {
    // ... verify the types match our expectations, generate a
    // compile-time error when they do not ...
    typedef typename attribute_type::tag_type tag_type;
    typedef typename attribute_type::value_type value_type;

    static_assert(
        std::is_same<attribute_type, attribute<tag_type, value_type>>::value,
        "security_attributes::set only works with its own attribute types");
    static_assert(
        std::is_convertible<U const&, value_type>::value,
        "security_attributes::set the input parameter must be convertible"
        " to the attribute type");

    // ... grow the vector to contain enough attributes ...
    attributes_.reserve(attribute_type::id + 1);
    // ... set the attribute, converting the @a t argument first ...
    attributes_[attribute_type::id] = boost::any(static_cast<value_type>(t));
  }

  /**
   * Get the value of an attribute
   *
   * @tparam attribute_type the tag identifying the attribute to set,
   * must be an instantiation of the @a attribute class
   * @returns the value of the @a attribute_type attribute
   * @throws boost::bad_any_cast if there is a type mismatch
   */
  template <typename attribute_type>
  typename attribute_type::value_type const& get() {
    // ... verify the types match our expectations, generate a
    // compile-time error when they do not ...
    typedef typename attribute_type::tag_type tag_type;
    typedef typename attribute_type::value_type value_type;

    // ... grow the array to contain enough attributes ...
    static_assert(
        std::is_same<attribute_type, attribute<tag_type, value_type>>::value,
        "security_attributes::set only works with its own attribute types");

    // ... grow the vector to contain enough attributes ...
    attributes_.reserve(attribute_type::id + 1);
    // ... return the attribute value ...
    return boost::any_cast<value_type const&>(attributes_[attribute_type::id]);
  }

  /// Generate a new id for an attribute
  static int generate_id() {
    // ... potentially we could relax the memory consistency
    // guarantees here, but it is not worth it, this function should
    // be called rarely, during the program initialization phase ...
    return id_generator.fetch_add(1);
  }

private:
  /// A generator for attribute ids
  static std::atomic<int> id_generator;

  /// The set of attributes
  std::vector<boost::any> attributes_;
};

template <typename tag>
std::atomic<int> security_attributes<tag>::id_generator = ATOMIC_VAR_INIT(0);

template <typename tag>
template <typename attribute_tag, typename value_t>
int const security_attributes<tag>::attribute<attribute_tag, value_t>::id =
    security_attributes<tag>::generate_id();

} // namespace jb

#endif // jb_security_attributes_hpp
