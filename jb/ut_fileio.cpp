#include <jb/fileio.hpp>

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>


/**
 * Implement the common part of the tests.
 */
void check_read_write(boost::filesystem::path const& path) {
  BOOST_TEST_MESSAGE("Using path=<" << path.string() << ">\n");

  std::vector<std::string> lines{
    "This is a sample file",
        "with more than one line",
        "yet entirely too short",
        };

  {
    boost::iostreams::filtering_ostream out;
    jb::open_output_file(out, path.string());
    for (auto const& line : lines) {
      BOOST_TEST_MESSAGE("Writing: " << line);
      out << line << "\n";
    }
  }

  {
    boost::iostreams::filtering_istream in;
    jb::open_input_file(in, path.string());
    for (auto const& expected : lines) {
      std::string got;
      std::getline(in, got);
      BOOST_CHECK_EQUAL(expected, got);
    }
  }

  boost::filesystem::remove(path);
}

/**
 * @test Verify we can read and write regular files...
 */
BOOST_AUTO_TEST_CASE(fileio_basic) {

  boost::filesystem::path tmp = boost::filesystem::temp_directory_path();
  tmp /= boost::filesystem::unique_path("%%%%-%%%%-%%%%.dat");

  check_read_write(tmp);
}

/**
 * @test Verify we can read and write regular files...
 */
BOOST_AUTO_TEST_CASE(fileio_gz) {

  boost::filesystem::path tmp = boost::filesystem::temp_directory_path();
  tmp /= boost::filesystem::unique_path("%%%%-%%%%-%%%%.gz");

  check_read_write(tmp);
}

/**
 * @test Verify we can write to stdout.
 */
BOOST_AUTO_TEST_CASE(fileio_stdout) {
  boost::iostreams::filtering_ostream out;
  BOOST_CHECK_NO_THROW(jb::open_output_file(out, "stdout"));
  out << "test message, please ignore\n";
}
