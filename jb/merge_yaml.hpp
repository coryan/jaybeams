#ifndef jb_merge_yaml_hpp
#define jb_merge_yaml_hpp

#include <yaml-cpp/yaml.h>
#include <map>

namespace jb {

/**
 * Store the overrides for each class.
 *
 * Jaybeams configuration objects can be overriden "by-class",
 * meaning, all configs of the same class receive the same overrides.
 * This type is used to (temporarily) store the by-class overrides in
 * a given context.
 */
typedef std::map<std::string, YAML::Node> class_overrides;

namespace yaml {

/**
 * Merge two YAML nodes.
 *
 * Unlike a simple assignment, if @a source does not have a value
 * for a given key, we keep the value from @a target.
 */
void merge_node(YAML::Node target, YAML::Node const& source);

/**
 * Merge all the values from @a source into @a target
 *
 * Unlike a simple assignment, if @a source does not have a value
 * for a given key, we keep the value from @a target.
 */
void merge_map(YAML::Node target, YAML::Node const& source);

/**
 * Memberwise merge two sequences, from @a source into @a target.
 *
 * If @a source has more elements than @a target the additional values
 * are appended.  If @a source has less elements than @a target, the
 * extra values in @a target are unmodified.
 */
void merge_sequences(YAML::Node target, YAML::Node const& source);

/**
 * Merge the class-overrides from @a source into @a by_class.
 *
 * Given a set of by-class overrides apply any additional by-class
 * overrides from @a source source into @a by_class.
 */
void merge(class_overrides& by_class, YAML::Node source);

/**
 * Recursively clone all the overrides in @a by_class.
 */
class_overrides clone(class_overrides const& by_class);

} // namespace yaml
} // namespace jb

#endif // jb_merge_yaml_hpp
