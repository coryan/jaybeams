#include <jb/merge_yaml.hpp>
#include <jb/assert_throw.hpp>

void jb::yaml::merge_node(YAML::Node target, YAML::Node const& source) {
  switch (source.Type()) {
  case YAML::NodeType::Scalar:
    target = source.Scalar();
    break;
  case YAML::NodeType::Map:
    merge_map(target, source);
    break;
  case YAML::NodeType::Sequence:
    merge_sequences(target, source);
    break;
  case YAML::NodeType::Null:
    throw std::runtime_error("merge_node: Null source nodes not supported");
  case YAML::NodeType::Undefined:
    throw std::runtime_error(
        "merge_node: Undefined source nodes not supported");
  }
}

void jb::yaml::merge_map(YAML::Node target, YAML::Node const& source) {
  for (auto const& j : source) {
    merge_node(target[j.first.Scalar()], j.second);
  }
}

void jb::yaml::merge_sequences(YAML::Node target, YAML::Node const& source) {
  for (std::size_t i = 0; i != source.size(); ++i) {
    if (i < target.size()) {
      merge_node(target[i], source[i]);
    } else {
      target.push_back(YAML::Clone(source[i]));
    }
  }
}

void jb::yaml::merge(class_overrides& by_class, YAML::Node node) {
  // Only Map nodes can override by-class values ...
  if (not node.IsMap()) {
    return;
  }
  // ... iterate over the node, searching for nodes with a key starting
  // with ':' ...
  for (auto i : node) {
    // ... the node is a map, there should be keys for all sub nodes ...
    JB_ASSERT_THROW(i.first);
    // ... found a key, check the format ...
    std::string key = i.first.as<std::string>();
    if (key.find(":") != 0) {
      continue;
    }
    // ... try to insert into the map ...
    auto ins = by_class.emplace(key, i.second);
    if (ins.second == true) {
      // ... good insert, nothing left to do ...
      continue;
    }
    // ... okay there was a node for the class in the map already,
    // need to merge the values ...
    merge_node(ins.first->second, i.second);
  }
}

jb::class_overrides jb::yaml::clone(class_overrides const& by_class) {
  class_overrides tmp;
  for (auto const& i : by_class) {
    tmp.emplace(i.first, YAML::Clone(i.second));
  }
  return tmp;
}
