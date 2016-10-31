#include <jb/merge_yaml.hpp>
#include <boost/test/unit_test.hpp>

#include <string>

/**
 * @test Verify merging of two maps.
 */
BOOST_AUTO_TEST_CASE(merge_yaml_map) {
  YAML::Node source = YAML::Load("{a: 1, b: 2}");
  YAML::Node target = YAML::Load("{a: 3, c: 4}");
  jb::yaml::merge_node(target, source);

  BOOST_CHECK_EQUAL(target["a"].as<std::string>(), "1");
  BOOST_CHECK_EQUAL(target["b"].as<std::string>(), "2");
  BOOST_CHECK_EQUAL(target["c"].as<std::string>(), "4");
}

/**
 * @test Verify merging of two sequences.
 */
BOOST_AUTO_TEST_CASE(merge_yaml_sequences) {
  YAML::Node source = YAML::Load("[11, 22, 33, 44]");
  YAML::Node ss = YAML::Load("[0, 1]");
  jb::yaml::merge_node(ss, source);

  auto actual = ss.as<std::vector<int>>();
  std::vector<int> expected({11, 22, 33, 44});
  BOOST_CHECK_EQUAL_COLLECTIONS(
      actual.begin(), actual.end(), expected.begin(), expected.end());

  YAML::Node ls = YAML::Load("[0, 1, 2, 3, 4, 5]");
  jb::yaml::merge_node(ls, source);

  actual = ls.as<std::vector<int>>();
  expected.assign({11, 22, 33, 44, 4, 5});
  BOOST_CHECK_EQUAL_COLLECTIONS(
      actual.begin(), actual.end(), expected.begin(), expected.end());
}

/**
 * @test Verify merging of sequences of maps
 */
BOOST_AUTO_TEST_CASE(merge_yaml_sequences_of_maps) {
  YAML::Node source = YAML::Load("[{a: 1}, {b: 2}, {a: 3, b: 3}, {c: 4}]");
  YAML::Node target = YAML::Load("[{}, {a: 11}, {c: 3}]");
  jb::yaml::merge_node(target, source);

  BOOST_REQUIRE_EQUAL(target.size(), 4);
  BOOST_CHECK_EQUAL(target[0]["a"].as<int>(), 1);
  BOOST_CHECK(not target[0]["b"].IsDefined());

  BOOST_CHECK_EQUAL(target[1]["a"].as<int>(), 11);
  BOOST_CHECK_EQUAL(target[1]["b"].as<int>(), 2);

  BOOST_CHECK_EQUAL(target[2]["a"].as<int>(), 3);
  BOOST_CHECK_EQUAL(target[2]["b"].as<int>(), 3);
  BOOST_CHECK_EQUAL(target[2]["c"].as<int>(), 3);

  BOOST_CHECK(not target[3]["a"].IsDefined());
  BOOST_CHECK_EQUAL(target[3]["c"].as<int>(), 4);
}

/**
 * @test Verify merging of two nested maps.
 */
BOOST_AUTO_TEST_CASE(merge_yaml_nested) {
  YAML::Node source = YAML::Load("{l0: {a: 1, b: 2}, l1: [1,2,3]}");
  YAML::Node target = YAML::Load("{l0: {a: 3, c: 4}, l2: [0,0]}");
  jb::yaml::merge_node(target, source);

  BOOST_CHECK_EQUAL(target["l0"]["a"].as<std::string>(), "1");
  BOOST_CHECK_EQUAL(target["l0"]["b"].as<std::string>(), "2");
  BOOST_CHECK_EQUAL(target["l0"]["c"].as<std::string>(), "4");
  BOOST_CHECK_EQUAL(target["l1"][0].as<int>(), 1);
  BOOST_CHECK_EQUAL(target["l1"][1].as<int>(), 2);
  BOOST_CHECK_EQUAL(target["l1"][2].as<int>(), 3);
}

/**
 * @test Verify merging of invalid nodes throws the right thing.
 */
BOOST_AUTO_TEST_CASE(merge_yaml_invalid_source) {
  YAML::Node target = YAML::Load("{a: 3, c: 4}");

  BOOST_CHECK_THROW(
      jb::yaml::merge_node(target, YAML::Node()), std::runtime_error);

  BOOST_CHECK_THROW(
      jb::yaml::merge_node(target, YAML::Load("{a: 1}")["b"]),
      std::runtime_error);
}
