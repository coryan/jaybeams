#include <jb/config_object.hpp>
#include <jb/config_files_location.hpp>
#include <jb/log.hpp>

#include <boost/program_options.hpp>
#include <fstream>
#include <iomanip>

namespace po = boost::program_options;

jb::config_object::config_object()
    : attributes_() {
}

void jb::config_object::load_overrides(
    int& argc, char* argv[], std::string const & filename,
    char const* environment_variable_name) {
  char argv0[] = "undefined";
  jb::config_files_locations<> search(
      argc > 1 ? argv[0] : argv0, environment_variable_name);
  try {
    auto full = search.find_configuration_file(filename);
    JB_LOG(debug) << "loading overrides from " << full;
    std::ifstream is(full.string());
    load_overrides(argc, argv, is);
    return;
  } catch(std::runtime_error const& ex) {
    JB_LOG(warning) << "fail to load overrides " << ex.what();
  }
  process_cmdline(argc, argv);
}

void jb::config_object::load_overrides(
    int& argc, char* argv[], std::string const & filename) {

  char argv0[] = "undefined";
  jb::config_files_locations<> search(argc > 1 ? argv[0] : argv0);
  try {
    auto full = search.find_configuration_file(filename);
    JB_LOG(debug) << "loading overrides from " << full;
    std::ifstream is(full.string());
    load_overrides(argc, argv, is);
    return;
  } catch(std::runtime_error const& ex) {
    JB_LOG(warning) << "fail to load overrides " << ex.what();
  }
  process_cmdline(argc, argv);
}

void jb::config_object::load_overrides(
    int& argc, char* argv[], std::istream& is) {
  YAML::Node doc = YAML::Load(is);
  apply_overrides(doc);
  process_cmdline(argc, argv);
}

void jb::config_object::apply_overrides(YAML::Node const& doc) {
  class_overrides by_class;
  merge(by_class, doc);
  apply_overrides(doc, by_class);
}

void jb::config_object::apply_overrides(
    YAML::Node const& by_name, class_overrides const& by_class) {
  for (auto i : attributes_) {
    if (i->class_name() != "") {
      std::string key = std::string(":") + i->class_name();
      auto n = by_class.find(key);
      if (n != by_class.end()) {
        i->apply_overrides(n->second, by_class);
      }
    }
    class_overrides new_scope = clone(by_class);
    merge(new_scope, by_name[i->name()]);
    i->apply_overrides(by_name[i->name()], new_scope);
  }
}

void jb::config_object::process_cmdline(int& argc, char* argv[]) {
  po::options_description options("Program Options");
  options.add_options()
      ("help", "produce help message")
      ("help-in-test", "produce help message (Boost.Test captures --help)")
      ;

  add_options(
      options, std::string(""), config_object::attribute_descriptor(""));

  // ... only top-level arguments get to be positional ...
  po::positional_options_description positional;
  for (auto i : attributes_) {
    if (i->positional()) {
      positional.add(i->name().c_str(), 1);
    }
  }

  po::variables_map vm;
  po::store(
      po::command_line_parser(argc, argv)
      .options(options)
      .positional(positional)
      .run(), vm);
  po::notify(vm);

  if (vm.count("help") or vm.count("help-in-test")) {
    std::ostringstream os;
    os << options << "\n";
    throw jb::usage{os.str(), 0};
  }
  
  apply_cmdline_values(vm, std::string(""));
  validate_all();
}

void jb::config_object::validate() const {
}

std::ostream& jb::config_object::to_stream(std::ostream& os) const {
  YAML::Node doc = to_yaml();
  return os << doc;
}

void jb::config_object::add_options(
    po::options_description& options, std::string const& prefix,
    attribute_descriptor const& d) const {
  po::options_description group(d.helpmsg);

  for (auto i : attributes_) {
    i->add_options(group, prefix, i->descriptor());
  }
  options.add(group);
}

void jb::config_object::apply_cmdline_values(
    po::variables_map const& vm, std::string const& prefix) {
  for (auto i : attributes_) {
    i->apply_cmdline_values(vm, cmdline_arg_name(prefix, i->name()));
  }
}

void jb::config_object::auto_register(attribute_base* a) {
  attributes_.push_back(a);
}

void jb::config_object::validate_all() const {
  validate();
  validate_attributes();
}

void jb::config_object::validate_attributes() const {
  for (auto i : attributes_) {
    i->validate();
  }
}

void jb::config_object::merge_values(
    YAML::Node target, YAML::Node const& source) {
  // TODO() this only works for relatively flat objects, we need to
  // recurse if the object
  for (auto const& j : source) {
    if (j.second.IsMap()) {
      merge_values(target[j.first.Scalar()], j.second);
    } else if (j.second.IsSequence()) {
      throw std::runtime_error("Not implemented");
    } else if (j.second.IsScalar()) {
      target[j.first.Scalar()] = j.second.Scalar();
    } else if (j.second.IsNull()) {
      throw std::runtime_error("Not implemented");
    }
  }
}

void jb::config_object::merge(
    class_overrides& by_class, YAML::Node node) {
  if (not node.IsMap()) {
    return;
  }
  // ... iterate over the node, searching for nodes with a key starting
  // with ':' ...
  for (auto i : node) {
    // ... no key, probably a sequence, skip ...
    if (not i.first) {
      continue;
    }
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
    merge_values(ins.first->second, i.second);
  }
}

jb::class_overrides jb::config_object::clone(class_overrides const& by_class) {
  class_overrides tmp;
  for (auto const& i : by_class) {
    tmp.emplace(i.first, YAML::Clone(i.second));
  }
  return std::move(tmp);
}

std::string jb::config_object::cmdline_arg_name(
    std::string const& prefix, std::string const& name) {
  if (prefix == "") {
    return name;
  }
  std::string tmp = prefix;
  tmp += ".";
  tmp += name;
  return std::move(tmp);
}

YAML::Node jb::config_object::to_yaml() const {
  YAML::Node doc;
  for (auto const& i : attributes_) {
    doc[i->name()] = i->to_yaml();
  }
  return doc;
}


jb::config_object::attribute_base::attribute_base(
    attribute_descriptor const& d, config_object* container)
    : descriptor_(d) {
  container->auto_register(this);
}

jb::config_object::attribute_base::~attribute_base() {
}