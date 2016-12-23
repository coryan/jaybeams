#include <jb/cpu_set.hpp>
#include <jb/convert_cpu_set.hpp>

#include <boost/test/unit_test.hpp>

#include <sstream>

/**
 * @test Verify that basic operations on jb::cpu_set work as expected.
 */
BOOST_AUTO_TEST_CASE(cpu_set_basic) {
  jb::cpu_set a;
  BOOST_REQUIRE_GT(a.capacity(), 0);

  BOOST_TEST_MESSAGE("cpu_set capacity is " << a.capacity());

  a.set(1);
  a.set(3);
  BOOST_CHECK_EQUAL(a.count(), 2);
  BOOST_CHECK_EQUAL(a.status(0), false);
  BOOST_CHECK_EQUAL(a.status(1), true);
  BOOST_CHECK_EQUAL(a.status(3), true);

  jb::cpu_set b(a);
  BOOST_CHECK_EQUAL(b.count(), 2);
  BOOST_CHECK_EQUAL(b.status(0), false);
  BOOST_CHECK_EQUAL(b.status(1), true);
  BOOST_CHECK_EQUAL(b.status(3), true);

  b.clear(3);
  BOOST_CHECK_EQUAL(b.status(3), false);
  BOOST_CHECK_EQUAL(a.count(), 2);
  BOOST_CHECK_EQUAL(b.count(), 1);

  jb::cpu_set c = std::move(a);
  BOOST_CHECK_EQUAL(c.count(), 2);
  BOOST_CHECK_EQUAL(c.status(0), false);
  BOOST_CHECK_EQUAL(c.status(1), true);
  BOOST_CHECK_EQUAL(c.status(3), true);

  a = std::move(b);
  BOOST_CHECK_EQUAL(a.count(), 1);
  BOOST_CHECK_EQUAL(a.status(0), false);
  BOOST_CHECK_EQUAL(a.status(1), true);
  BOOST_CHECK_EQUAL(a.status(3), false);

  b = c;
  BOOST_CHECK_EQUAL(b.count(), 2);
  BOOST_CHECK_EQUAL(b.status(0), false);
  BOOST_CHECK_EQUAL(b.status(1), true);
  BOOST_CHECK_EQUAL(b.status(3), true);

  a.set(1, 5);
  BOOST_CHECK_EQUAL(a.count(), 5);
  for (int i = 1; i != 6; ++i) {
    BOOST_CHECK_MESSAGE(a.status(i) == true, "b.status(i) is false i=" << i);
  }

  BOOST_CHECK_THROW(b.set(b.capacity()), std::exception);
  BOOST_CHECK_THROW(b.set(b.capacity() + 1), std::exception);
  BOOST_CHECK_THROW(b.set(-1), std::exception);
  BOOST_CHECK_THROW(b.set(0, b.capacity()), std::exception);
  BOOST_CHECK_NO_THROW(b.set(0, b.capacity() - 1));

  a.reset().set(1).set(2).set(3);
  b = a;
  BOOST_CHECK_EQUAL(a, b);
  b.set(10);
  BOOST_CHECK_NE(a, b);

  a.reset();
  BOOST_CHECK_EQUAL(a.count(), 0);

  b.reset().set(2).set(3);
  c.reset().set(0).set(3);

  a = b | c;
  BOOST_CHECK_EQUAL(a.status(0), true);
  BOOST_CHECK_EQUAL(a.status(1), false);
  BOOST_CHECK_EQUAL(a.status(2), true);
  BOOST_CHECK_EQUAL(a.status(3), true);

  a = b & c;
  BOOST_CHECK_EQUAL(a.status(0), false);
  BOOST_CHECK_EQUAL(a.status(1), false);
  BOOST_CHECK_EQUAL(a.status(2), false);
  BOOST_CHECK_EQUAL(a.status(3), true);

  a = b ^ c;
  BOOST_CHECK_EQUAL(a.status(0), true);
  BOOST_CHECK_EQUAL(a.status(1), false);
  BOOST_CHECK_EQUAL(a.status(2), true);
  BOOST_CHECK_EQUAL(a.status(3), false);
}

/**
 * @test Verify that jb::cpu_set output streaming works as expected.
 */
BOOST_AUTO_TEST_CASE(cpu_set_ostream) {
  {
    jb::cpu_set a;
    std::ostringstream os;
    os << a;
    BOOST_CHECK_EQUAL(os.str(), std::string(""));
  }

  {
    jb::cpu_set a;
    a.set(1);
    std::ostringstream os;
    os << a;
    BOOST_CHECK_EQUAL(os.str(), std::string("1"));
  }

  {
    jb::cpu_set a;
    a.set(1, 5);
    std::ostringstream os;
    os << a;
    BOOST_CHECK_EQUAL(os.str(), std::string("1-5"));
  }

  {
    jb::cpu_set a;
    a.set(1, 5);
    a.set(7);
    std::ostringstream os;
    os << a;
    BOOST_CHECK_EQUAL(os.str(), std::string("1-5,7"));
  }

  {
    jb::cpu_set a;
    a.set(10, 200);
    a.set(7);
    a.set(11);
    a.set(1, 5);
    a.set(300);
    a.set(301);
    std::ostringstream os;
    os << a;
    BOOST_CHECK_EQUAL(os.str(), std::string("1-5,7,10-200,300-301"));
  }
}

/**
 * @test Verify that jb::cpu_set input streaming works as expected.
 */
BOOST_AUTO_TEST_CASE(cpu_set_istream) {
  {
    std::istringstream is("");
    jb::cpu_set a;
    is >> a;
    BOOST_CHECK_EQUAL(a.count(), 0);
  }

  {
    std::istringstream is("1");
    jb::cpu_set a;
    a.set(1);
    BOOST_CHECK_EQUAL(a.count(), 1);
    BOOST_CHECK_EQUAL(a.status(1), true);
  }

  {
    jb::cpu_set a;
    std::istringstream is("1-5");
    is >> a;
    BOOST_CHECK_EQUAL(a.count(), 5);
    for (int i = 1; i != 6; ++i) {
      BOOST_CHECK_MESSAGE(a.status(i), "a.status(i) not true for i=" << i);
    }
  }

  {
    jb::cpu_set a;
    std::istringstream is("1-5,7");
    is >> a;
    BOOST_CHECK_EQUAL(a.count(), 6);
    for (int i = 1; i != 6; ++i) {
      BOOST_CHECK_MESSAGE(a.status(i), "a.status(i) not true for i=" << i);
    }
    BOOST_CHECK_EQUAL(a.status(7), true);
  }

  {
    jb::cpu_set a;
    std::istringstream is("1-5,7,10-200,300-301");
    is >> a;
    BOOST_CHECK_EQUAL(a.count(), 199);
    for (int i = 1; i != 6; ++i) {
      BOOST_CHECK_MESSAGE(a.status(i), "a.status(i) not true for i=" << i);
    }
    for (int i = 10; i != 200; ++i) {
      BOOST_CHECK_MESSAGE(a.status(i), "a.status(i) not true for i=" << i);
    }
    BOOST_CHECK_EQUAL(a.status(7), true);
    BOOST_CHECK_EQUAL(a.status(300), true);
    BOOST_CHECK_EQUAL(a.status(301), true);
  }
}

/**
 * @test Verify that the clear() operation works as expected for a range.
 */
BOOST_AUTO_TEST_CASE(cpu_set_clear) {
  jb::cpu_set a;
  a.set(2);
  a.set(3);
  a.set(4);
  BOOST_CHECK_EQUAL(a.count(), 3);
  BOOST_CHECK_EQUAL(a.status(3), true);
  a.clear(2, 4);
  BOOST_CHECK_EQUAL(a.count(), 0);
  BOOST_CHECK_EQUAL(a.status(3), false);
}

/**
 * @test Verify that the parse() function works as expected for
 * invalid inputs.
 */
BOOST_AUTO_TEST_CASE(cpu_set_parse_invalid) {
  BOOST_CHECK_THROW(jb::cpu_set::parse("zzz"), std::exception);
  BOOST_CHECK_THROW(jb::cpu_set::parse("1-zzz"), std::exception);
  BOOST_CHECK_THROW(jb::cpu_set::parse("zzz-2"), std::exception);
  BOOST_CHECK_THROW(jb::cpu_set::parse("1-2-zzz"), std::exception);
  BOOST_CHECK_THROW(jb::cpu_set::parse("1-2-3"), std::exception);
  BOOST_CHECK_THROW(jb::cpu_set::parse("-"), std::exception);
  BOOST_CHECK_THROW(jb::cpu_set::parse("--"), std::exception);
  BOOST_CHECK_NO_THROW(jb::cpu_set::parse("1-2"));
}

/**
 * @test Verify that the YAML conversion functions work as expected.
 */
BOOST_AUTO_TEST_CASE(cpu_set_yaml_convert) {
  jb::cpu_set empty;
  YAML::Node encoded = YAML::convert<jb::cpu_set>::encode(empty);
  BOOST_CHECK_EQUAL(encoded.as<std::string>(), std::string(""));
  jb::cpu_set decoded;
  YAML::convert<jb::cpu_set>::decode(encoded, decoded);
  BOOST_CHECK_EQUAL(empty, decoded);
  
  jb::cpu_set a = jb::cpu_set::parse("1,3-5");
  encoded = YAML::convert<jb::cpu_set>::encode(a);
  BOOST_CHECK_EQUAL(encoded.as<std::string>(), std::string("1,3-5"));
  YAML::convert<jb::cpu_set>::decode(encoded, decoded);
  BOOST_CHECK_EQUAL(a, decoded);
}
