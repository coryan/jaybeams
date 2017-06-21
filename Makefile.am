# JayBeams uses a non-recursive make, this is the only Makefile.
#
# Copyright 2015-2017 Carlos O'Ryan
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

instdir = $(prefix)

lib_LIBRARIES = \
  jb/libjb.a \
  jb/libjb_ehs.a \
  jb/libjb_etcd.a \
  jb/libjb_itch5.a \
  jb/libjb_pitch2.a \
  jb/libjb_testing.a \
  jb/libjb_itch5_testing.a

# The unit tests are added (via +=) in the same section where we
# define the sources and headers for each library
unit_tests = 

# ... same thing for the benchmarks ...
benchmarks =

# ... and the examples ...
examples =

bin_SCRIPTS = tools/benchmark_common.sh

bin_PROGRAMS = $(benchmarks) \
  jb/itch5/mold2inside \
  jb/itch5/moldfeedhandler \
  jb/itch5/moldreplay \
  jb/testing/show_compile_info \
  tools/itch5bookdepth \
  tools/itch5eventdepth \
  tools/itch5inside \
  tools/itch5moldreplay \
  tools/itch5stats \
  tools/itch5trades \
  tools/moldheartbeat

noinst_PROGRAMS = $(examples) \
  jb/testing/check_mt19937_initializer \
  jb/testing/check_random_device

check_PROGRAMS = $(unit_tests)
TESTS = $(check_PROGRAMS)

@VALGRIND_CHECK_RULES@

AM_CXXFLAGS = @AM_CXXFLAGS@
AM_CXXFLAGS += \
  $(PTHREAD_CFLAGS)

AM_CPPFLAGS = @AM_CPPFLAGS@
AM_CPPFLAGS += \
  -DJB_SYSCONFDIR=\"$(sysconfdir)\" \
  -DJB_BINDIR=\"$(bindir)\" \
  $(YAML_CPP_CPPFLAGS) \
  $(BOOST_CPPFLAGS)

LDFLAGS =

if STATIC_LINKING
LDFLAGS += -Wl,-Bstatic
else
AM_CXXFLAGS += -DBOOST_LOG_DYN_LINK 
endif !STATIC_LINKING

YAML_CPP_LIBS=@YAML_CPP_LIBS@

LIBS = \
  $(YAML_CPP_LDFLAGS) \
  $(YAML_CPP_LIBS) \
  $(BOOST_LDFLAGS) \
  $(BOOST_LOG_SETUP_LIB) \
  $(BOOST_LOG_LIB) \
  $(BOOST_FILESYSTEM_LIB) \
  $(BOOST_SYSTEM_LIB) \
  $(BOOST_DATE_TIME_LIB) \
  $(BOOST_THREAD_LIB) \
  $(BOOST_PROGRAM_OPTIONS_LIB) \
  $(BOOST_IOSTREAMS_LIB) \
  $(BZIP2_LDFLAGS) \
  $(BZIP2_LIBS) \
  $(LIBZ_LDFLAGS) \
  $(LIBZ_LIBS) \
  $(PTHREAD_LIBS)

# Common configuration for all unit tests
UT_CPPFLAGS = \
  -DBOOST_TEST_MAIN \
  -DSKYE_USE_BOOST_UNIT_TEST_FRAMEWORK \
  $(CPPFLAGS)

if STATIC_LINKING
LIBS += -Wl,-Bdynamic
else
UT_CPPFLAGS += -DBOOST_TEST_DYN_LINK
endif !STATIC_LINKING

jb_ut_ldadd = \
  jb/libjb.a \
  $(BOOST_UNIT_TEST_FRAMEWORK_LIB)
jb_ut_ldflags =

################################################################
# If CLANG_FORMAT was found, 
################################################################
if FOUND_CLANG_FORMAT
check-style:
	$(AM_V_at)find $(top_srcdir) -name '*.[hic]pp' -type f \
	  ! -path $(top_srcdir)'/etcd/*' \
	  ! -path $(top_srcdir)'/gogoproto/*' \
	  ! -path $(top_srcdir)'/googleapis/*' | \
	  xargs $(CLANG_FORMAT) -i
	$(AM_V_at)git diff --exit-code && echo "clang-format checks passed"
else
check-style:
	$(AM_V_at)echo "clang-format not found, cannot check style" && exit 1
endif !FOUND_CLANG_FORMAT
.PHONY: check-style

################################################################
# Special rules to compile protocol buffer files
# TODO(#157) - this introduces a dependency on GNU Make.  We just
# accept it as a given.
################################################################

# Define the variables to silently compile ProtoBuf files into cpp
# files.
AM_PROTOCFLAGS =
PROTOC_COMPILE = $(PROTOC) $(PROTOCFLAGS) $(AM_PROTOCFLAGS)
AM_V_PROTOC = $(am__v_PROTOC_$(V))
am__v_PROTOC_ = $(am__v_PROTOC_$(AM_DEFAULT_VERBOSITY))
am__v_PROTOC_0 = @echo "  PROTOC  " $@;
am__v_PROTOC_1 = 

# TODO(#158) - define the search path for protocol buffers
JB_AM_PROTO_SEARCH_PATHS=$(srcdir)
vpath %.proto $(JB_AM_PROTO_SEARCH_PATHS)

%.pb.cc %.pb.h: %.proto
	$(AM_V_PROTOC)$(PROTOC_COMPILE)	--cpp_out=$(builddir) $^

# Define the variables to generate gRPC C++ files
GRPC_COMPILE = $(PROTOC_COMPILE) --plugin=protoc-gen-grpc="$(GRPC_CPP_PLUGIN)"
AM_V_GRPC = $(am__v_GRPC_$(V))
am__v_GRPC_ = $(am__v_GRPC_$(AM_DEFAULT_VERBOSITY))
am__v_GRPC_0 = @echo "  GRPC    " $@;
am__v_GRPC_1 = 

%.grpc.pb.cc %.grpc.pb.h: %.proto
	$(AM_V_GRPC)$(GRPC_COMPILE) --grpc_out=$(builddir) --cpp_out=$(builddir) $^

################################################################
# jb/libjb.a
################################################################

jb_libjb_adir = $(includedir)
nobase_jb_libjb_a_HEADERS = \
  jb/as_hhmmss.hpp \
  jb/assert_throw.hpp \
  jb/book_depth_statistics.hpp \
  jb/complex_traits.hpp \
  jb/config_attribute.hpp \
  jb/config_files_location.hpp \
  jb/config_object.hpp \
  jb/config_recurse.hpp \
  jb/convert_cpu_set.hpp \
  jb/convert_severity_level.hpp \
  jb/cpu_set.hpp \
  jb/detail/array_traits.hpp \
  jb/detail/os_error.hpp \
  jb/detail/reconfigure_thread.hpp \
  jb/detail/thread_setup_wrapper.hpp \
  jb/event_rate_estimator.hpp \
  jb/event_rate_histogram.hpp \
  jb/explicit_cuts_binning.hpp \
  jb/feed_error.hpp \
  jb/fileio.hpp \
  jb/filetype.hpp \
  jb/fixed_string.hpp \
  jb/histogram.hpp \
  jb/histogram_summary.hpp \
  jb/integer_range_binning.hpp \
  jb/launch_thread.hpp \
  jb/log.hpp \
  jb/merge_yaml.hpp \
  jb/offline_feed_statistics.hpp \
  jb/p2ceil.hpp \
  jb/severity_level.hpp \
  jb/strtonum.hpp \
  jb/thread_config.hpp \
  jb/usage.hpp

jb_libjb_a_SOURCES = \
  jb/as_hhmmss.cpp \
  jb/assert_throw.cpp \
  jb/book_depth_statistics.cpp \
  jb/config_files_location.cpp \
  jb/config_object.cpp \
  jb/cpu_set.cpp \
  jb/detail/os_error.cpp \
  jb/detail/reconfigure_thread.cpp \
  jb/fileio.cpp \
  jb/filetype.cpp \
  jb/histogram_summary.cpp \
  jb/log.cpp \
  jb/merge_yaml.cpp \
  jb/offline_feed_statistics.cpp \
  jb/severity_level.cpp \
  jb/thread_config.cpp

unit_tests += \
  jb/ut_as_hhmmss \
  jb/ut_assert_throw \
  jb/ut_book_depth_statistics \
  jb/ut_config_files_location \
  jb/ut_config_object \
  jb/ut_config_object_vector \
  jb/ut_cpu_set \
  jb/ut_event_rate_estimator \
  jb/ut_event_rate_histogram \
  jb/ut_explicit_cuts_binning \
  jb/ut_fileio \
  jb/ut_filetype \
  jb/ut_fixed_string \
  jb/ut_histogram \
  jb/ut_histogram_summary \
  jb/ut_integer_range_binning \
  jb/ut_launch_thread \
  jb/ut_logging \
  jb/ut_merge_yaml \
  jb/ut_offline_feed_statistics \
  jb/ut_p2ceil \
  jb/ut_severity_level \
  jb/ut_strtonum \
  jb/ut_thread_config

benchmarks += jb/bm_clocks

examples += examples/configuration

################################################################
# jb/libjb_ehs.a and its unit tests
################################################################
jb_libjb_ehs_adir = $(includedir)
jb_libjb_ehs_a_SOURCES = \
  jb/ehs/acceptor.cpp \
  jb/ehs/connection.cpp \
  jb/ehs/request_dispatcher.cpp
nobase_jb_libjb_ehs_a_HEADERS = \
  jb/ehs/acceptor.hpp \
  jb/ehs/base_types.hpp \
  jb/ehs/connection.hpp \
  jb/ehs/request_dispatcher.hpp

unit_tests += \
  jb/ehs/ut_acceptor \
  jb/ehs/ut_request_dispatcher

jb_ut_ehs_ldadd = jb/libjb_ehs.a $(jb_ut_ldadd)
jb_ut_ehs_ldflags = $(jb_ut_ldflags)

jb_ehs_ut_acceptor_SOURCES = jb/ehs/ut_acceptor.cpp
jb_ehs_ut_acceptor_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ehs_ut_acceptor
jb_ehs_ut_acceptor_LDFLAGS = $(jb_ut_ehs_ldflags)
jb_ehs_ut_acceptor_LDADD = $(jb_ut_ehs_ldadd)

jb_ehs_ut_request_dispatcher_SOURCES = jb/ehs/ut_request_dispatcher.cpp
jb_ehs_ut_request_dispatcher_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ehs_ut_request_dispatcher
jb_ehs_ut_request_dispatcher_LDFLAGS = $(jb_ut_ehs_ldflags)
jb_ehs_ut_request_dispatcher_LDADD = $(jb_ut_ehs_ldadd)

################################################################
# jb/libjb_itch5_testing.a
################################################################

jb_ut_testing_ldadd = \
  jb/libjb_testing.a \
  $(jb_ut_ldadd)
jb_ut_testing_ldflags = \
  $(jb_ut_ldflags)

examples_configuration_SOURCES = examples/configuration.cpp
examples_configuration_LDADD = jb/libjb.a

jb_bm_clocks_SOURCES = jb/bm_clocks.cpp
jb_bm_clocks_CPPFLAGS =
jb_bm_clocks_LDADD = jb/libjb_testing.a jb/libjb.a

jb_ut_as_hhmmss_SOURCES = jb/ut_as_hhmmss.cpp
jb_ut_as_hhmmss_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_as_hhmmss
jb_ut_as_hhmmss_LDFLAGS = $(jb_ut_ldflags)
jb_ut_as_hhmmss_LDADD = $(jb_ut_ldadd)

jb_ut_assert_throw_SOURCES = jb/ut_assert_throw.cpp
jb_ut_assert_throw_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_assert_throw
jb_ut_assert_throw_LDFLAGS = $(jb_ut_ldflags)
jb_ut_assert_throw_LDADD = $(jb_ut_ldadd)

jb_ut_book_depth_statistics_SOURCES = jb/ut_book_depth_statistics.cpp
jb_ut_book_depth_statistics_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_book_depth_statistics
jb_ut_book_depth_statistics_LDFLAGS = $(jb_ut_ldflags)
jb_ut_book_depth_statistics_LDADD = $(jb_ut_ldadd)

jb_ut_config_files_location_SOURCES = jb/ut_config_files_location.cpp
jb_ut_config_files_location_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_config_files_location
jb_ut_config_files_location_LDFLAGS = $(jb_ut_ldflags)
jb_ut_config_files_location_LDADD = $(jb_ut_ldadd)

jb_ut_config_object_SOURCES = jb/ut_config_object.cpp
jb_ut_config_object_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_config_object
jb_ut_config_object_LDFLAGS = $(jb_ut_ldflags)
jb_ut_config_object_LDADD = $(jb_ut_ldadd)

jb_ut_config_object_vector_SOURCES = jb/ut_config_object_vector.cpp
jb_ut_config_object_vector_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_config_object_vector
jb_ut_config_object_vector_LDFLAGS = $(jb_ut_ldflags)
jb_ut_config_object_vector_LDADD = $(jb_ut_ldadd)

jb_ut_cpu_set_SOURCES = jb/ut_cpu_set.cpp
jb_ut_cpu_set_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_cpu_set
jb_ut_cpu_set_LDFLAGS = $(jb_ut_ldflags)
jb_ut_cpu_set_LDADD = $(jb_ut_ldadd)

jb_ut_event_rate_estimator_SOURCES = jb/ut_event_rate_estimator.cpp
jb_ut_event_rate_estimator_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_event_rate_estimator
jb_ut_event_rate_estimator_LDFLAGS = $(jb_ut_ldflags)
jb_ut_event_rate_estimator_LDADD = $(jb_ut_ldadd)

jb_ut_event_rate_histogram_SOURCES = jb/ut_event_rate_histogram.cpp
jb_ut_event_rate_histogram_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_event_rate_histogram
jb_ut_event_rate_histogram_LDFLAGS = $(jb_ut_ldflags)
jb_ut_event_rate_histogram_LDADD = $(jb_ut_ldadd)

jb_ut_explicit_cuts_binning_SOURCES = jb/ut_explicit_cuts_binning.cpp
jb_ut_explicit_cuts_binning_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_explicit_cuts_binning
jb_ut_explicit_cuts_binning_LDFLAGS = $(jb_ut_ldflags)
jb_ut_explicit_cuts_binning_LDADD = $(jb_ut_ldadd)

jb_ut_fileio_SOURCES = jb/ut_fileio.cpp
jb_ut_fileio_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_fileio
jb_ut_fileio_LDFLAGS = $(jb_ut_ldflags)
jb_ut_fileio_LDADD = $(jb_ut_ldadd)

jb_ut_filetype_SOURCES = jb/ut_filetype.cpp
jb_ut_filetype_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_filetype
jb_ut_filetype_LDFLAGS = $(jb_ut_ldflags)
jb_ut_filetype_LDADD = $(jb_ut_ldadd)

jb_ut_fixed_string_SOURCES = jb/ut_fixed_string.cpp
jb_ut_fixed_string_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_fixed_string
jb_ut_fixed_string_LDFLAGS = $(jb_ut_ldflags)
jb_ut_fixed_string_LDADD = $(jb_ut_ldadd)

jb_ut_histogram_SOURCES = jb/ut_histogram.cpp
jb_ut_histogram_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_histogram
jb_ut_histogram_LDFLAGS = $(jb_ut_ldflags)
jb_ut_histogram_LDADD = $(jb_ut_ldadd)

jb_ut_histogram_summary_SOURCES = jb/ut_histogram.cpp
jb_ut_histogram_summary_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_histogram_summary
jb_ut_histogram_summary_LDFLAGS = $(jb_ut_ldflags)
jb_ut_histogram_summary_LDADD = $(jb_ut_ldadd)

jb_ut_integer_range_binning_SOURCES = jb/ut_integer_range_binning.cpp
jb_ut_integer_range_binning_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_integer_range_binning
jb_ut_integer_range_binning_LDFLAGS = $(jb_ut_ldflags)
jb_ut_integer_range_binning_LDADD = $(jb_ut_ldadd)

jb_ut_launch_thread_SOURCES = jb/ut_launch_thread.cpp
jb_ut_launch_thread_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_launch_thread
jb_ut_launch_thread_LDFLAGS = $(jb_ut_ldflags)
jb_ut_launch_thread_LDADD = $(jb_ut_ldadd)

jb_ut_logging_SOURCES = jb/ut_logging.cpp
jb_ut_logging_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_logging
jb_ut_logging_LDFLAGS = $(jb_ut_ldflags)
jb_ut_logging_LDADD = $(jb_ut_ldadd)

jb_ut_merge_yaml_SOURCES = jb/ut_merge_yaml.cpp
jb_ut_merge_yaml_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_merge_yaml
jb_ut_merge_yaml_LDFLAGS = $(jb_ut_ldflags)
jb_ut_merge_yaml_LDADD = $(jb_ut_ldadd)

jb_ut_p2ceil_SOURCES = jb/ut_p2ceil.cpp
jb_ut_p2ceil_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_p2ceil
jb_ut_p2ceil_LDFLAGS = $(jb_ut_ldflags)
jb_ut_p2ceil_LDADD = $(jb_ut_ldadd)

jb_ut_offline_feed_statistics_SOURCES = jb/ut_offline_feed_statistics.cpp
jb_ut_offline_feed_statistics_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_offline_feed_statistics
jb_ut_offline_feed_statistics_LDFLAGS = $(jb_ut_ldflags)
jb_ut_offline_feed_statistics_LDADD = $(jb_ut_ldadd)

jb_ut_severity_level_SOURCES = jb/ut_severity_level.cpp
jb_ut_severity_level_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_severity_level
jb_ut_severity_level_LDFLAGS = $(jb_ut_ldflags)
jb_ut_severity_level_LDADD = $(jb_ut_ldadd)

jb_ut_strtonum_SOURCES = jb/ut_strtonum.cpp
jb_ut_strtonum_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_strtonum
jb_ut_strtonum_LDFLAGS = $(jb_ut_ldflags)
jb_ut_strtonum_LDADD = $(jb_ut_ldadd)

jb_ut_thread_config_SOURCES = jb/ut_thread_config.cpp
jb_ut_thread_config_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_thread_config
jb_ut_thread_config_LDFLAGS = $(jb_ut_ldflags)
jb_ut_thread_config_LDADD = $(jb_ut_ldadd)

################################################################
# jb/libjb_testing.a
################################################################

jb_libjb_testing_adir = $(includedir)
jb_libjb_testing_a_SOURCES = \
  jb/testing/compile_info.cpp \
  jb/testing/microbenchmark_base.cpp \
  jb/testing/microbenchmark_config.cpp \
  jb/testing/microbenchmark_group_main.cpp

nobase_jb_libjb_testing_a_HEADERS = \
  jb/testing/check_close_enough.hpp \
  jb/testing/create_square_timeseries.hpp \
  jb/testing/create_triangle_timeseries.hpp \
  jb/testing/delay_timeseries.hpp \
  jb/testing/initialize_mersenne_twister.hpp \
  jb/testing/microbenchmark.hpp \
  jb/testing/microbenchmark_base.hpp \
  jb/testing/microbenchmark_config.hpp \
  jb/testing/microbenchmark_group_main.hpp \
  jb/testing/sum_square.hpp

jb/testing/compile_info.cpp: jb/testing/compile_info.hpp Makefile
	$(AM_V_GEN)echo '#include <jb/testing/compile_info.hpp>' >$@; \
	echo '' >>$@; \
	echo 'char const jb::testing::uname_a[] = R"""(' >>$@; \
	uname -a >>$@; \
	echo ')""";' >> $@; \
	echo '' >>$@; \
	echo 'char const jb::testing::compiler[] = R"""(' >>$@; \
	$(CXX) --version >>$@; \
	echo ')""";' >> $@; \
	echo '' >>$@; \
	echo 'char const jb::testing::compiler_flags[] = R"""(' >>$@; \
	echo $(AM_CXXFLAGS) $(CXXFLAGS) >>$@; \
	echo ')""";' >> $@; \
	echo '' >>$@; \
	echo 'char const jb::testing::linker[] = R"""(' >>$@; \
	$(CXX) -Wl,--version >>$@ 2>/dev/null; \
	echo ')""";' >> $@; \
	echo '' >>$@; \
	echo 'char const jb::testing::gitrev[] = R"""(' >>$@; \
	git rev-parse HEAD >>$@ 2>/dev/null; \
	echo ')""";' >> $@; \
	echo '' >>$@

unit_tests += \
  jb/testing/ut_check_close_enough \
  jb/testing/ut_create_square_timeseries \
  jb/testing/ut_create_triangle_timeseries \
  jb/testing/ut_delay_timeseries \
  jb/testing/ut_microbenchmark_config

jb_testing_check_mt19937_initializer_SOURCES = jb/testing/check_mt19937_initializer.cpp
jb_testing_check_mt19937_initializer_LDADD = jb/libjb.a

jb_testing_check_random_device_SOURCES = jb/testing/check_random_device.cpp
jb_testing_check_random_device_LDADD = jb/libjb.a

jb_testing_show_compile_info_SOURCES = jb/testing/show_compile_info.cpp
jb_testing_show_compile_info_LDADD = jb/libjb_testing.a jb/libjb.a

jb_testing_ut_check_close_enough_SOURCES = jb/testing/ut_check_close_enough.cpp
jb_testing_ut_check_close_enough_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_testing_ut_check_close_enough
jb_testing_ut_check_close_enough_LDADD = $(jb_ut_testing_ldadd)

jb_testing_ut_create_square_timeseries_SOURCES = jb/testing/ut_create_square_timeseries.cpp
jb_testing_ut_create_square_timeseries_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_testing_ut_create_square_timeseries
jb_testing_ut_create_square_timeseries_LDFLAGS = $(jb_ut_testing_ldflags)
jb_testing_ut_create_square_timeseries_LDADD = $(jb_ut_testing_ldadd)

jb_testing_ut_create_triangle_timeseries_SOURCES = jb/testing/ut_create_triangle_timeseries.cpp
jb_testing_ut_create_triangle_timeseries_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_testing_ut_create_triangle_timeseries
jb_testing_ut_create_triangle_timeseries_LDFLAGS = $(jb_ut_testing_ldflags)
jb_testing_ut_create_triangle_timeseries_LDADD = $(jb_ut_testing_ldadd)

jb_testing_ut_delay_timeseries_SOURCES = jb/testing/ut_delay_timeseries.cpp
jb_testing_ut_delay_timeseries_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_testing_ut_delay_timeseries
jb_testing_ut_delay_timeseries_LDFLAGS = $(jb_ut_testing_ldflags)
jb_testing_ut_delay_timeseries_LDADD = $(jb_ut_testing_ldadd)

jb_testing_ut_microbenchmark_config_SOURCES = jb/testing/ut_microbenchmark_config.cpp
jb_testing_ut_microbenchmark_config_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_testing_ut_microbenchmark_config
jb_testing_ut_microbenchmark_config_LDFLAGS = $(jb_ut_testing_ldflags)
jb_testing_ut_microbenchmark_config_LDADD = $(jb_ut_testing_ldadd)

################################################################
# jb/libjb_etcd.a
################################################################


# TODO(#158) - the settings of these flags carefully workaround the
# layout structure expected by protoc(1) and the differences between
# how googleapis and etcd layout their files and import their protos.
AM_PROTOCFLAGS += -I$(srcdir)/googleapis -I$(srcdir)/gogoproto -I$(srcdir)
JB_AM_PROTO_SEARCH_PATHS += $(srcdir)/googleapis $(srcdir)/gogoproto
AM_CXXFLAGS += -I$(builddir)/gogoproto -I$(builddir)/googleapis -I$(builddir)

jb_libjb_etcd_adir = $(includedir)
jb_libjb_etcd_a_SOURCES = \
  google/api/http.pb.cc \
  google/api/annotations.pb.cc \
  gogoproto/gogo.pb.cc \
  etcd/mvcc/mvccpb/kv.pb.cc \
  etcd/auth/authpb/auth.pb.cc \
  etcd/etcdserver/etcdserverpb/rpc.grpc.pb.cc \
  etcd/etcdserver/etcdserverpb/rpc.pb.cc
nobase_jb_libjb_etcd_a_HEADERS = \
  google/api/http.pb.h \
  google/api/annotations.pb.h \
  gogoproto/gogo.pb.h \
  etcd/mvcc/mvccpb/kv.pb.h \
  etcd/auth/authpb/auth.pb.h \
  etcd/etcdserver/etcdserverpb/rpc.grpc.pb.h \
  etcd/etcdserver/etcdserverpb/rpc.pb.h

noinst_PROGRAMS += \
  jb/etcd/election_listener \
  jb/etcd/election_participant

# TODO() - decide if the protobuf files should be included in the
# distribution tarball ...
nodist_noinst_DATA = \
  googleapis/google/api/http.proto \
  googleapis/google/api/annotations.proto \
  gogoproto/gogoproto/gogo.proto \
  etcd/mvcc/mvccpb/kv.proto \
  etcd/auth/authpb/auth.proto \
  etcd/etcdserver/etcdserverpb/rpc.proto

jb_etcd_election_listener_SOURCES = \
  jb/etcd/election_listener.cpp
jb_etcd_election_listener_CPPFLAGS = $(GRPCPP_CPPFLAGS) \
  $(PROTOBUF_CPPFLAGS) -I$(builddir)/gogoproto -I$(builddir)/googleapis
jb_etcd_election_listener_CXXFLAGS = $(GRPCPP_CFLAGS) $(PROTOBUF_CFLAGS)
jb_etcd_election_listener_LDADD = \
    jb/libjb_etcd.a jb/libjb.a $(GRPCPP_LIBS) $(PROTOBUF_LIBS)
jb/etcd/jb_etcd_election_listener-election_listener.$(OBJEXT): $(nobase_jb_libjb_etcd_a_HEADERS)

jb_etcd_election_participant_SOURCES = \
  jb/etcd/election_participant.cpp
jb_etcd_election_participant_CPPFLAGS = $(GRPCPP_CPPFLAGS) \
  $(PROTOBUF_CPPFLAGS) -I$(builddir)/gogoproto -I$(builddir)/googleapis
jb_etcd_election_participant_CXXFLAGS = $(GRPCPP_CFLAGS) $(PROTOBUF_CFLAGS)
jb_etcd_election_participant_LDADD = \
    jb/libjb_etcd.a jb/libjb.a $(GRPCPP_LIBS) $(PROTOBUF_LIBS)
jb/etcd/jb_etcd_election_participant-election_participant.$(OBJEXT): $(nobase_jb_libjb_etcd_a_HEADERS)

################################################################
# jb/libjb_itch5_testing.a
################################################################

jb_libjb_itch5_testing_adir = $(includedir)
jb_libjb_itch5_testing_a_SOURCES = \
  jb/itch5/testing/data.cpp \
  jb/itch5/testing/messages.cpp
nobase_jb_libjb_itch5_testing_a_HEADERS = \
  jb/itch5/testing/data.hpp \
  jb/itch5/testing/messages.hpp

################################################################
# jb/libjb_itch5.a
################################################################

jb_libjb_itch5_adir = $(includedir)
jb_libjb_itch5_a_SOURCES = \
  jb/itch5/add_order_message.cpp \
  jb/itch5/add_order_mpid_message.cpp \
  jb/itch5/array_based_order_book.cpp \
  jb/itch5/base_decoders.cpp \
  jb/itch5/broken_trade_message.cpp \
  jb/itch5/char_list_validator.cpp \
  jb/itch5/check_offset.cpp \
  jb/itch5/cross_trade_message.cpp \
  jb/itch5/ipo_quoting_period_update_message.cpp \
  jb/itch5/market_participant_position_message.cpp \
  jb/itch5/message_header.cpp \
  jb/itch5/mold_udp_channel.cpp \
  jb/itch5/mold_udp_pacer_config.cpp \
  jb/itch5/mwcb_breach_message.cpp \
  jb/itch5/mwcb_decline_level_message.cpp \
  jb/itch5/net_order_imbalance_indicator_message.cpp \
  jb/itch5/order_cancel_message.cpp \
  jb/itch5/order_delete_message.cpp \
  jb/itch5/order_executed_message.cpp \
  jb/itch5/order_executed_price_message.cpp \
  jb/itch5/order_replace_message.cpp \
  jb/itch5/reg_sho_restriction_message.cpp \
  jb/itch5/seconds_field.cpp \
  jb/itch5/stock_directory_message.cpp \
  jb/itch5/stock_trading_action_message.cpp \
  jb/itch5/system_event_message.cpp \
  jb/itch5/timestamp.cpp \
  jb/itch5/trade_message.cpp \
  jb/itch5/udp_config_common.cpp \
  jb/itch5/udp_receiver_config.cpp \
  jb/itch5/udp_sender_config.cpp

nobase_jb_libjb_itch5_a_HEADERS = \
  jb/itch5/add_order_message.hpp \
  jb/itch5/add_order_mpid_message.hpp \
  jb/itch5/array_based_order_book.hpp \
  jb/itch5/base_decoders.hpp \
  jb/itch5/base_encoders.hpp \
  jb/itch5/broken_trade_message.hpp \
  jb/itch5/char_list_field.hpp \
  jb/itch5/char_list_validator.hpp \
  jb/itch5/check_offset.hpp \
  jb/itch5/compute_book.hpp \
  jb/itch5/cross_trade_message.hpp \
  jb/itch5/cross_type.hpp \
  jb/itch5/decoder.hpp \
  jb/itch5/generate_inside.hpp \
  jb/itch5/quote_defaults.hpp \
  jb/itch5/ipo_quoting_period_update_message.hpp \
  jb/itch5/make_socket_udp_common.hpp \
  jb/itch5/make_socket_udp_recv.hpp \
  jb/itch5/make_socket_udp_send.hpp \
  jb/itch5/map_based_order_book.hpp \
  jb/itch5/market_participant_position_message.hpp \
  jb/itch5/message_header.hpp \
  jb/itch5/mold_udp_channel.hpp \
  jb/itch5/mold_udp_pacer.hpp \
  jb/itch5/mold_udp_pacer_config.hpp \
  jb/itch5/mold_udp_protocol_constants.hpp \
  jb/itch5/mpid_field.hpp \
  jb/itch5/mwcb_breach_message.hpp \
  jb/itch5/mwcb_decline_level_message.hpp \
  jb/itch5/net_order_imbalance_indicator_message.hpp \
  jb/itch5/order_book.hpp \
  jb/itch5/order_cancel_message.hpp \
  jb/itch5/order_delete_message.hpp \
  jb/itch5/order_executed_message.hpp \
  jb/itch5/order_executed_price_message.hpp \
  jb/itch5/order_replace_message.hpp \
  jb/itch5/price_field.hpp \
  jb/itch5/price_levels.hpp \
  jb/itch5/process_buffer_mlist.hpp \
  jb/itch5/process_iostream_mlist.hpp \
  jb/itch5/process_iostream.hpp \
  jb/itch5/protocol_constants.hpp \
  jb/itch5/reg_sho_restriction_message.hpp \
  jb/itch5/seconds_field.hpp \
  jb/itch5/short_string_field.hpp \
  jb/itch5/static_digits.hpp \
  jb/itch5/stock_directory_message.hpp \
  jb/itch5/stock_field.hpp \
  jb/itch5/stock_trading_action_message.hpp \
  jb/itch5/system_event_message.hpp \
  jb/itch5/timestamp.hpp \
  jb/itch5/trade_message.hpp \
  jb/itch5/udp_config_common.hpp \
  jb/itch5/udp_sender_config.hpp \
  jb/itch5/udp_receiver_config.hpp \
  jb/itch5/unknown_message.hpp

unit_tests += \
  jb/itch5/ut_add_order_message \
  jb/itch5/ut_add_order_mpid_message \
  jb/itch5/ut_array_based_order_book \
  jb/itch5/ut_base_decoders \
  jb/itch5/ut_base_encoders \
  jb/itch5/ut_broken_trade_message \
  jb/itch5/ut_char_list_field \
  jb/itch5/ut_char_list_validator \
  jb/itch5/ut_check_offset \
  jb/itch5/ut_compute_book \
  jb/itch5/ut_cross_trade_message \
  jb/itch5/ut_cross_type \
  jb/itch5/ut_generate_inside \
  jb/itch5/ut_ipo_quoting_period_update_message \
  jb/itch5/ut_make_socket_udp_common \
  jb/itch5/ut_make_socket_udp_recv \
  jb/itch5/ut_make_socket_udp_send \
  jb/itch5/ut_map_based_order_book \
  jb/itch5/ut_market_participant_position_message \
  jb/itch5/ut_message_header \
  jb/itch5/ut_mold_udp_pacer \
  jb/itch5/ut_mold_udp_pacer_config \
  jb/itch5/ut_mold_udp_channel \
  jb/itch5/ut_mwcb_breach_message \
  jb/itch5/ut_mwcb_decline_level_message \
  jb/itch5/ut_net_order_imbalance_indicator_message \
  jb/itch5/ut_order_book \
  jb/itch5/ut_order_cancel_message \
  jb/itch5/ut_order_delete_message \
  jb/itch5/ut_order_executed_message \
  jb/itch5/ut_order_executed_price_message \
  jb/itch5/ut_order_replace_message \
  jb/itch5/ut_price_field \
  jb/itch5/ut_price_levels \
  jb/itch5/ut_process_buffer_mlist \
  jb/itch5/ut_process_iostream_mlist \
  jb/itch5/ut_reg_sho_restriction_message \
  jb/itch5/ut_seconds_field \
  jb/itch5/ut_short_string_field \
  jb/itch5/ut_static_digits \
  jb/itch5/ut_stock_directory_message \
  jb/itch5/ut_stock_trading_action_message \
  jb/itch5/ut_system_event_message \
  jb/itch5/ut_timestamp \
  jb/itch5/ut_trade_message \
  jb/itch5/ut_udp_receiver_config

benchmarks += jb/itch5/bm_order_book
bin_SCRIPTS += \
  jb/itch5/bm_order_book_generate.sh \
  jb/itch5/bm_order_book_analyze.R \
  jb/itch5/bm_order_book_report.Rmd

jb_ut_itch5_ldadd = \
  jb/libjb_itch5_testing.a \
  jb/libjb_itch5.a \
  $(jb_ut_ldadd)
jb_ut_itch5_ldflags = \
  $(jb_ut_ldflags)

################################################################
# jb/libjb_pitch2.a
################################################################

jb_libjb_pitch2_adir = $(includedir)
jb_libjb_pitch2_a_SOURCES = \
  jb/pitch2/add_order_message.cpp \
  jb/pitch2/auction_update_message.cpp \
  jb/pitch2/delete_order_message.cpp \
  jb/pitch2/expanded_add_order_message.cpp \
  jb/pitch2/modify_message.cpp \
  jb/pitch2/order_executed_message.cpp \
  jb/pitch2/order_executed_price_message.cpp \
  jb/pitch2/reduce_size_message.cpp \
  jb/pitch2/short_add_order_message.cpp \
  jb/pitch2/time_message.cpp \
  jb/pitch2/unit_clear_message.cpp
jb_libjb_pitch2_a_HEADERS = \
  jb/pitch2/add_order_message.hpp \
  jb/pitch2/auction_update_message.hpp \
  jb/pitch2/base_add_order_message.hpp \
  jb/pitch2/base_add_order_message_streaming.hpp \
  jb/pitch2/delete_order_message.hpp \
  jb/pitch2/expanded_add_order_message.hpp \
  jb/pitch2/modify_message.hpp \
  jb/pitch2/order_executed_message.hpp \
  jb/pitch2/order_executed_price_message.hpp \
  jb/pitch2/reduce_size_message.hpp \
  jb/pitch2/short_add_order_message.hpp \
  jb/pitch2/time_message.hpp \
  jb/pitch2/unit_clear_message.hpp

unit_tests += \
  jb/pitch2/ut_add_order_message \
  jb/pitch2/ut_auction_update_message \
  jb/pitch2/ut_delete_order_message \
  jb/pitch2/ut_expanded_add_order_message \
  jb/pitch2/ut_modify_message \
  jb/pitch2/ut_order_executed_message \
  jb/pitch2/ut_order_executed_price_message \
  jb/pitch2/ut_reduce_size_message \
  jb/pitch2/ut_short_add_order_message \
  jb/pitch2/ut_time_message \
  jb/pitch2/ut_unit_clear_message

jb_ut_pitch2_ldadd = \
  jb/libjb_pitch2.a \
  $(jb_ut_ldadd)
jb_ut_pitch2_ldflags = \
  $(jb_ut_ldflags)

#
# pitch2 unit tests
#

jb_pitch2_ut_add_order_message_SOURCES = jb/pitch2/ut_add_order_message.cpp
jb_pitch2_ut_add_order_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_pitch2_ut_add_order_message
jb_pitch2_ut_add_order_message_LDFLAGS = $(jb_ut_pitch2_ldflags)
jb_pitch2_ut_add_order_message_LDADD = $(jb_ut_pitch2_ldadd)

jb_pitch2_ut_auction_update_message_SOURCES = jb/pitch2/ut_auction_update_message.cpp
jb_pitch2_ut_auction_update_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_pitch2_ut_auction_update_message
jb_pitch2_ut_auction_update_message_LDFLAGS = $(jb_ut_pitch2_ldflags)
jb_pitch2_ut_auction_update_message_LDADD = $(jb_ut_pitch2_ldadd)

jb_pitch2_ut_delete_order_message_SOURCES = jb/pitch2/ut_delete_order_message.cpp
jb_pitch2_ut_delete_order_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_pitch2_ut_delete_order_message
jb_pitch2_ut_delete_order_message_LDFLAGS = $(jb_ut_pitch2_ldflags)
jb_pitch2_ut_delete_order_message_LDADD = $(jb_ut_pitch2_ldadd)

jb_pitch2_ut_expanded_add_order_message_SOURCES = jb/pitch2/ut_expanded_add_order_message.cpp
jb_pitch2_ut_expanded_add_order_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_pitch2_ut_expanded_add_order_message
jb_pitch2_ut_expanded_add_order_message_LDFLAGS = $(jb_ut_pitch2_ldflags)
jb_pitch2_ut_expanded_add_order_message_LDADD = $(jb_ut_pitch2_ldadd)

jb_pitch2_ut_modify_message_SOURCES = jb/pitch2/ut_modify_message.cpp
jb_pitch2_ut_modify_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_pitch2_ut_modify_message
jb_pitch2_ut_modify_message_LDFLAGS = $(jb_ut_pitch2_ldflags)
jb_pitch2_ut_modify_message_LDADD = $(jb_ut_pitch2_ldadd)

jb_pitch2_ut_order_executed_message_SOURCES = jb/pitch2/ut_order_executed_message.cpp
jb_pitch2_ut_order_executed_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_pitch2_ut_order_executed_message
jb_pitch2_ut_order_executed_message_LDFLAGS = $(jb_ut_pitch2_ldflags)
jb_pitch2_ut_order_executed_message_LDADD = $(jb_ut_pitch2_ldadd)

jb_pitch2_ut_order_executed_price_message_SOURCES = jb/pitch2/ut_order_executed_price_message.cpp
jb_pitch2_ut_order_executed_price_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_pitch2_ut_order_executed_price_message
jb_pitch2_ut_order_executed_price_message_LDFLAGS = $(jb_ut_pitch2_ldflags)
jb_pitch2_ut_order_executed_price_message_LDADD = $(jb_ut_pitch2_ldadd)

jb_pitch2_ut_reduce_size_message_SOURCES = jb/pitch2/ut_reduce_size_message.cpp
jb_pitch2_ut_reduce_size_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_pitch2_ut_reduce_size_message
jb_pitch2_ut_reduce_size_message_LDFLAGS = $(jb_ut_pitch2_ldflags)
jb_pitch2_ut_reduce_size_message_LDADD = $(jb_ut_pitch2_ldadd)

jb_pitch2_ut_short_add_order_message_SOURCES = jb/pitch2/ut_short_add_order_message.cpp
jb_pitch2_ut_short_add_order_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_pitch2_ut_short_add_order_message
jb_pitch2_ut_short_add_order_message_LDFLAGS = $(jb_ut_pitch2_ldflags)
jb_pitch2_ut_short_add_order_message_LDADD = $(jb_ut_pitch2_ldadd)

jb_pitch2_ut_time_message_SOURCES = jb/pitch2/ut_time_message.cpp
jb_pitch2_ut_time_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_pitch2_ut_time_message
jb_pitch2_ut_time_message_LDFLAGS = $(jb_ut_pitch2_ldflags)
jb_pitch2_ut_time_message_LDADD = $(jb_ut_pitch2_ldadd)

jb_pitch2_ut_unit_clear_message_SOURCES = jb/pitch2/ut_unit_clear_message.cpp
jb_pitch2_ut_unit_clear_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_pitch2_ut_unit_clear_message
jb_pitch2_ut_unit_clear_message_LDFLAGS = $(jb_ut_pitch2_ldflags)
jb_pitch2_ut_unit_clear_message_LDADD = $(jb_ut_pitch2_ldadd)

################################################################
# examples
################################################################

tools_itch5bookdepth_SOURCES = tools/itch5bookdepth.cpp
tools_itch5bookdepth_LDADD = jb/libjb_itch5.a jb/libjb.a

tools_itch5eventdepth_SOURCES = tools/itch5eventdepth.cpp
tools_itch5eventdepth_LDADD = jb/libjb_itch5.a jb/libjb.a

tools_itch5inside_SOURCES = tools/itch5inside.cpp
tools_itch5inside_LDADD = jb/libjb_itch5.a jb/libjb.a

tools_itch5moldreplay_SOURCES = tools/itch5moldreplay.cpp
tools_itch5moldreplay_LDFLAGS = $(BOOST_ASIO_LIB)
tools_itch5moldreplay_LDADD = jb/libjb_itch5.a jb/libjb.a

tools_itch5stats_SOURCES = tools/itch5stats.cpp
tools_itch5stats_LDADD = jb/libjb_itch5.a jb/libjb.a

tools_itch5trades_SOURCES = tools/itch5trades.cpp
tools_itch5trades_LDADD = jb/libjb_itch5.a jb/libjb.a

tools_moldheartbeat_SOURCES = tools/moldheartbeat.cpp
tools_moldheartbeat_LDFLAGS = $(BOOST_ASIO_LIBS)
tools_moldheartbeat_LDADD = jb/libjb_itch5.a jb/libjb.a

jb_itch5_mold2inside_SOURCES = jb/itch5/mold2inside.cpp
jb_itch5_mold2inside_LDFLAGS = $(BOOST_ASIO_LIBS)
jb_itch5_mold2inside_LDADD = jb/libjb_itch5.a jb/libjb.a

jb_itch5_moldreplay_SOURCES = jb/itch5/moldreplay.cpp
jb_itch5_moldreplay_LDFLAGS = $(BOOST_ASIO_LIBS)
jb_itch5_moldreplay_LDADD = jb/libjb_itch5.a jb/libjb_ehs.a jb/libjb.a

jb_itch5_moldfeedhandler_SOURCES = jb/itch5/moldfeedhandler.cpp
jb_itch5_moldfeedhandler_LDFLAGS = $(BOOST_ASIO_LIBS)
jb_itch5_moldfeedhandler_LDADD = jb/libjb_itch5.a jb/libjb_ehs.a jb/libjb.a

jb_itch5_ut_add_order_message_SOURCES = jb/itch5/ut_add_order_message.cpp
jb_itch5_ut_add_order_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_add_order_message
jb_itch5_ut_add_order_message_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_add_order_message_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_add_order_mpid_message_SOURCES = jb/itch5/ut_add_order_mpid_message.cpp
jb_itch5_ut_add_order_mpid_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_add_order_mpid_messag
jb_itch5_ut_add_order_mpid_message_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_add_order_mpid_message_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_array_based_order_book_SOURCES = jb/itch5/ut_array_based_order_book.cpp
jb_itch5_ut_array_based_order_book_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_array_based_order_book
jb_itch5_ut_array_based_order_book_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_array_based_order_book_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_base_decoders_SOURCES = jb/itch5/ut_base_decoders.cpp
jb_itch5_ut_base_decoders_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_base_decoders
jb_itch5_ut_base_decoders_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_base_decoders_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_base_encoders_SOURCES = jb/itch5/ut_base_encoders.cpp
jb_itch5_ut_base_encoders_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_base_encoders
jb_itch5_ut_base_encoders_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_base_encoders_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_broken_trade_message_SOURCES = jb/itch5/ut_broken_trade_message.cpp
jb_itch5_ut_broken_trade_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_broken_trade_message
jb_itch5_ut_broken_trade_message_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_broken_trade_message_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_char_list_field_SOURCES = jb/itch5/ut_char_list_field.cpp
jb_itch5_ut_char_list_field_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_char_list_field
jb_itch5_ut_char_list_field_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_char_list_field_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_char_list_validator_SOURCES = jb/itch5/ut_char_list_validator.cpp
jb_itch5_ut_char_list_validator_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_char_list_validator
jb_itch5_ut_char_list_validator_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_char_list_validator_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_check_offset_SOURCES = jb/itch5/ut_check_offset.cpp
jb_itch5_ut_check_offset_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_check_offset
jb_itch5_ut_check_offset_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_check_offset_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_compute_book_SOURCES = jb/itch5/ut_compute_book.cpp
jb_itch5_ut_compute_book_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_compute_book
jb_itch5_ut_compute_book_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_compute_book_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_cross_trade_message_SOURCES = jb/itch5/ut_cross_trade_message.cpp
jb_itch5_ut_cross_trade_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_cross_trade_message
jb_itch5_ut_cross_trade_message_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_cross_trade_message_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_cross_type_SOURCES = jb/itch5/ut_cross_type.cpp
jb_itch5_ut_cross_type_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_cross_type
jb_itch5_ut_cross_type_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_cross_type_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_generate_inside_SOURCES = jb/itch5/ut_generate_inside.cpp
jb_itch5_ut_generate_inside_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_generate_inside
jb_itch5_ut_generate_inside_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_generate_inside_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_ipo_quoting_period_update_message_SOURCES = jb/itch5/ut_ipo_quoting_period_update_message.cpp
jb_itch5_ut_ipo_quoting_period_update_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_ipo_quoting_period_update_message
jb_itch5_ut_ipo_quoting_period_update_message_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_ipo_quoting_period_update_message_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_make_socket_udp_common_SOURCES = jb/itch5/ut_make_socket_udp_common.cpp
jb_itch5_ut_make_socket_udp_common_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_make_socket_udp_common
jb_itch5_ut_make_socket_udp_common_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_make_socket_udp_common_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_make_socket_udp_recv_SOURCES = jb/itch5/ut_make_socket_udp_recv.cpp
jb_itch5_ut_make_socket_udp_recv_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_make_socket_udp_recv
jb_itch5_ut_make_socket_udp_recv_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_make_socket_udp_recv_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_make_socket_udp_send_SOURCES = jb/itch5/ut_make_socket_udp_send.cpp
jb_itch5_ut_make_socket_udp_send_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_make_socket_udp_send
jb_itch5_ut_make_socket_udp_send_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_make_socket_udp_send_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_map_based_order_book_SOURCES = jb/itch5/ut_map_based_order_book.cpp
jb_itch5_ut_map_based_order_book_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_map_based_order_book
jb_itch5_ut_map_based_order_book_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_map_based_order_book_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_market_participant_position_message_SOURCES = jb/itch5/ut_market_participant_position_message.cpp
jb_itch5_ut_market_participant_position_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_market_participant_position_message
jb_itch5_ut_market_participant_position_message_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_market_participant_position_message_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_message_header_SOURCES = jb/itch5/ut_message_header.cpp
jb_itch5_ut_message_header_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_message_header
jb_itch5_ut_message_header_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_message_header_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_mold_udp_channel_SOURCES = jb/itch5/ut_mold_udp_channel.cpp
jb_itch5_ut_mold_udp_channel_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_mold_udp_channel
jb_itch5_ut_mold_udp_channel_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_mold_udp_channel_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_mold_udp_pacer_SOURCES = jb/itch5/ut_mold_udp_pacer.cpp
jb_itch5_ut_mold_udp_pacer_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_mold_udp_pacer
jb_itch5_ut_mold_udp_pacer_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_mold_udp_pacer_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_mold_udp_pacer_config_SOURCES = jb/itch5/ut_mold_udp_pacer_config.cpp
jb_itch5_ut_mold_udp_pacer_config_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_mold_udp_pacer_config
jb_itch5_ut_mold_udp_pacer_config_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_mold_udp_pacer_config_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_mwcb_breach_message_SOURCES = jb/itch5/ut_mwcb_breach_message.cpp
jb_itch5_ut_mwcb_breach_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_mwcb_breach_message
jb_itch5_ut_mwcb_breach_message_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_mwcb_breach_message_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_mwcb_decline_level_message_SOURCES = jb/itch5/ut_mwcb_decline_level_message.cpp
jb_itch5_ut_mwcb_decline_level_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_mwcb_decline_level_message
jb_itch5_ut_mwcb_decline_level_message_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_mwcb_decline_level_message_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_net_order_imbalance_indicator_message_SOURCES = jb/itch5/ut_net_order_imbalance_indicator_message.cpp
jb_itch5_ut_net_order_imbalance_indicator_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_net_order_imbalance_indicator_message
jb_itch5_ut_net_order_imbalance_indicator_message_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_net_order_imbalance_indicator_message_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_order_book_SOURCES = jb/itch5/ut_order_book.cpp
jb_itch5_ut_order_book_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_order_book
jb_itch5_ut_order_book_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_order_book_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_order_cancel_message_SOURCES = jb/itch5/ut_order_cancel_message.cpp
jb_itch5_ut_order_cancel_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_order_cancel_message
jb_itch5_ut_order_cancel_message_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_order_cancel_message_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_order_delete_message_SOURCES = jb/itch5/ut_order_delete_message.cpp
jb_itch5_ut_order_delete_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_order_delete_message
jb_itch5_ut_order_delete_message_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_order_delete_message_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_order_executed_message_SOURCES = jb/itch5/ut_order_executed_message.cpp
jb_itch5_ut_order_executed_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_order_executed_message
jb_itch5_ut_order_executed_message_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_order_executed_message_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_order_executed_price_message_SOURCES = jb/itch5/ut_order_executed_price_message.cpp
jb_itch5_ut_order_executed_price_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_order_executed_price_message
jb_itch5_ut_order_executed_price_message_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_order_executed_price_message_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_order_replace_message_SOURCES = jb/itch5/ut_order_replace_message.cpp
jb_itch5_ut_order_replace_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_order_replace_message
jb_itch5_ut_order_replace_message_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_order_replace_message_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_price_field_SOURCES = jb/itch5/ut_price_field.cpp
jb_itch5_ut_price_field_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_price_field
jb_itch5_ut_price_field_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_price_field_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_price_levels_SOURCES = jb/itch5/ut_price_levels.cpp
jb_itch5_ut_price_levels_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_price_levels
jb_itch5_ut_price_levels_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_price_levels_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_process_buffer_mlist_SOURCES = jb/itch5/ut_process_buffer_mlist.cpp
jb_itch5_ut_process_buffer_mlist_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_process_buffer_mlist
jb_itch5_ut_process_buffer_mlist_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_process_buffer_mlist_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_process_iostream_mlist_SOURCES = jb/itch5/ut_process_iostream_mlist.cpp
jb_itch5_ut_process_iostream_mlist_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_process_iostream_mlist
jb_itch5_ut_process_iostream_mlist_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_process_iostream_mlist_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_reg_sho_restriction_message_SOURCES = jb/itch5/ut_reg_sho_restriction_message.cpp
jb_itch5_ut_reg_sho_restriction_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_reg_sho_restriction_message
jb_itch5_ut_reg_sho_restriction_message_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_reg_sho_restriction_message_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_seconds_field_SOURCES = jb/itch5/ut_seconds_field.cpp
jb_itch5_ut_seconds_field_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_seconds_field
jb_itch5_ut_seconds_field_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_seconds_field_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_short_string_field_SOURCES = jb/itch5/ut_short_string_field.cpp
jb_itch5_ut_short_string_field_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_short_string_field
jb_itch5_ut_short_string_field_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_short_string_field_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_static_digits_SOURCES = jb/itch5/ut_static_digits.cpp
jb_itch5_ut_static_digits_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_static_digits
jb_itch5_ut_static_digits_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_static_digits_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_stock_directory_message_SOURCES = jb/itch5/ut_stock_directory_message.cpp
jb_itch5_ut_stock_directory_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_stock_directory_message
jb_itch5_ut_stock_directory_message_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_stock_directory_message_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_stock_trading_action_message_SOURCES = jb/itch5/ut_stock_trading_action_message.cpp
jb_itch5_ut_stock_trading_action_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_stock_trading_action_message
jb_itch5_ut_stock_trading_action_message_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_stock_trading_action_message_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_system_event_message_SOURCES = jb/itch5/ut_system_event_message.cpp
jb_itch5_ut_system_event_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_system_event_message
jb_itch5_ut_system_event_message_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_system_event_message_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_timestamp_SOURCES = jb/itch5/ut_timestamp.cpp
jb_itch5_ut_timestamp_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_timestamp
jb_itch5_ut_timestamp_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_timestamp_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_trade_message_SOURCES = jb/itch5/ut_trade_message.cpp
jb_itch5_ut_trade_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_trade_message
jb_itch5_ut_trade_message_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_trade_message_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_ut_udp_receiver_config_SOURCES = jb/itch5/ut_udp_receiver_config.cpp
jb_itch5_ut_udp_receiver_config_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_udp_receiver_config
jb_itch5_ut_udp_receiver_config_LDFLAGS = $(jb_ut_itch5_ldflags)
jb_itch5_ut_udp_receiver_config_LDADD = $(jb_ut_itch5_ldadd)

jb_itch5_bm_order_book_SOURCES = jb/itch5/bm_order_book.cpp
jb_itch5_bm_order_book_CPPFLAGS = 
jb_itch5_bm_order_book_LDADD = jb/libjb_itch5.a jb/libjb_testing.a jb/libjb.a

################################################################
# Optional FFTW stuff
################################################################

if USE_FFTW3
# jb/libjb_fftw.a
lib_LIBRARIES += jb/libjb_fftw.a
unit_tests += jb/fftw/ut_aligned_multi_array \
  jb/fftw/ut_allocator \
  jb/fftw/ut_cast_multi_array \
  jb/fftw/ut_cast_vector \
  jb/fftw/ut_plan \
  jb/fftw/ut_plan_many \
  jb/fftw/ut_tde_result \
  jb/fftw/ut_time_delay_estimator \
  jb/fftw/ut_time_delay_estimator_many \
  jb/fftw/ut_traits

benchmarks += jb/fftw/bm_time_delay_estimator \
  jb/fftw/bm_time_delay_estimator_many

bin_SCRIPTS += jb/fftw/bm_time_delay_estimator_alignment.sh

AM_CPPFLAGS += $(FFTW3_CPPFLAGS)

jb_ut_fftw_ldadd = \
  jb/libjb_testing.a \
  jb/libjb_fftw.a \
  $(FFTW3_LDFLAGS) $(FFTW3_LIBS) \
  $(jb_ut_ldadd)

jb_ut_fftw_ldflags = \
  $(jb_ut_ldflags)

else
jb_ut_fftw_ldadd =
jb_ut_fftw_ldflags =
endif !USE_FFTW3

jb_libjb_fftw_adir = $(includedir)
jb_libjb_fftw_a_SOURCES = \
  jb/fftw/plan.cpp
nobase_jb_libjb_fftw_a_HEADERS = \
  jb/fftw/aligned_multi_array.hpp \
  jb/fftw/aligned_vector.hpp \
  jb/fftw/allocator.hpp \
  jb/fftw/cast.hpp \
  jb/fftw/plan.hpp \
  jb/fftw/tde_result.hpp \
  jb/fftw/time_delay_estimator.hpp \
  jb/fftw/time_delay_estimator_many.hpp \
  jb/fftw/traits.hpp

jb_fftw_ut_aligned_multi_array_SOURCES = jb/fftw/ut_aligned_multi_array.cpp
jb_fftw_ut_aligned_multi_array_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_fftw_ut_aligned_multi_array
jb_fftw_ut_aligned_multi_array_LDFLAGS = $(jb_ut_fftw_ldflags)
jb_fftw_ut_aligned_multi_array_LDADD = $(jb_ut_fftw_ldadd)

jb_fftw_ut_allocator_SOURCES = jb/fftw/ut_allocator.cpp
jb_fftw_ut_allocator_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_fftw_ut_allocator
jb_fftw_ut_allocator_LDFLAGS = $(jb_ut_fftw_ldflags)
jb_fftw_ut_allocator_LDADD = $(jb_ut_fftw_ldadd)

jb_fftw_ut_cast_multi_array_SOURCES = jb/fftw/ut_cast_multi_array.cpp
jb_fftw_ut_cast_multi_array_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_fftw_ut_cast_multi_array
jb_fftw_ut_cast_multi_array_LDFLAGS = $(jb_ut_fftw_ldflags)
jb_fftw_ut_cast_multi_array_LDADD = $(jb_ut_fftw_ldadd)

jb_fftw_ut_cast_vector_SOURCES = jb/fftw/ut_cast_vector.cpp
jb_fftw_ut_cast_vector_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_fftw_ut_cast_vector
jb_fftw_ut_cast_vector_LDFLAGS = $(jb_ut_fftw_ldflags)
jb_fftw_ut_cast_vector_LDADD = $(jb_ut_fftw_ldadd)

jb_fftw_ut_plan_SOURCES = jb/fftw/ut_plan.cpp
jb_fftw_ut_plan_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_fftw_ut_plan
jb_fftw_ut_plan_LDFLAGS = $(jb_ut_fftw_ldflags)
jb_fftw_ut_plan_LDADD = $(jb_ut_fftw_ldadd)

jb_fftw_ut_plan_many_SOURCES = jb/fftw/ut_plan_many.cpp
jb_fftw_ut_plan_many_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_fftw_ut_plan_many
jb_fftw_ut_plan_many_LDFLAGS = $(jb_ut_fftw_ldflags)
jb_fftw_ut_plan_many_LDADD = $(jb_ut_fftw_ldadd)

jb_fftw_ut_tde_result_SOURCES = jb/fftw/ut_tde_result.cpp
jb_fftw_ut_tde_result_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_fftw_ut_tde_result
jb_fftw_ut_tde_result_LDFLAGS = $(jb_ut_fftw_ldflags)
jb_fftw_ut_tde_result_LDADD = $(jb_ut_fftw_ldadd)

jb_fftw_ut_time_delay_estimator_SOURCES = jb/fftw/ut_time_delay_estimator.cpp
jb_fftw_ut_time_delay_estimator_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_fftw_ut_time_delay_estimator
jb_fftw_ut_time_delay_estimator_LDFLAGS = $(jb_ut_fftw_ldflags)
jb_fftw_ut_time_delay_estimator_LDADD = $(jb_ut_fftw_ldadd)

jb_fftw_ut_time_delay_estimator_many_SOURCES = jb/fftw/ut_time_delay_estimator_many.cpp
jb_fftw_ut_time_delay_estimator_many_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_fftw_ut_time_delay_estimator_many
jb_fftw_ut_time_delay_estimator_many_LDFLAGS = $(jb_ut_fftw_ldflags)
jb_fftw_ut_time_delay_estimator_many_LDADD = $(jb_ut_fftw_ldadd)

jb_fftw_ut_traits_SOURCES = jb/fftw/ut_traits.cpp
jb_fftw_ut_traits_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_fftw_ut_traits
jb_fftw_ut_traits_LDFLAGS = $(jb_ut_fftw_ldflags)
jb_fftw_ut_traits_LDADD = $(jb_ut_fftw_ldadd)

jb_fftw_bm_time_delay_estimator_SOURCES = jb/fftw/bm_time_delay_estimator.cpp
jb_fftw_bm_time_delay_estimator_CPPFLAGS =
jb_fftw_bm_time_delay_estimator_LDADD = \
  jb/libjb_testing.a \
  jb/libjb_fftw.a \
  jb/libjb.a \
  $(FFTW3_LIBS)
jb_fftw_bm_time_delay_estimator_LDFLAGS = \
  $(FFTW3_LDFLAGS)

jb_fftw_bm_time_delay_estimator_many_SOURCES = jb/fftw/bm_time_delay_estimator_many.cpp
jb_fftw_bm_time_delay_estimator_many_CPPFLAGS =
jb_fftw_bm_time_delay_estimator_many_LDADD = \
  jb/libjb_testing.a \
  jb/libjb_fftw.a \
  jb/libjb.a \
  $(FFTW3_LIBS)
jb_fftw_bm_time_delay_estimator_many_LDFLAGS = \
  $(FFTW3_LDFLAGS)

################################################################
# Conditionally enable the OpenCL components
################################################################
jb_libjb_tde_adir = $(includedir)
jb_libjb_tde_a_SOURCES =
nobase_jb_libjb_tde_a_HEADERS =

if HAVE_OPENCL
lib_LIBRARIES += \
  jb/libjb_opencl.a \
  jb/libjb_opencl_testing.a

jb_opencl_unit_tests = \
  jb/opencl/ut_build_simple_kernel \
  jb/opencl/ut_config \
  jb/opencl/ut_device_selector \
  jb/opencl/ut_generic_reduce
jb_opencl_examples = \
  jb/opencl/show_device
jb_opencl_benchmarks = \
  jb/opencl/bm_copy_buffer \
  jb/opencl/bm_generic_reduce \
  jb/opencl/bm_launch_kernel

jb_ut_opencl_ldadd = \
  jb/libjb_opencl.a \
  $(jb_ut_ldadd) \
  $(OPENCL_LIB)
jb_ut_opencl_ldflags = \
  $(OPENCL_LDFLAGS)

lib_LIBRARIES += jb/libjb_clfft.a
jb_clfft_unit_tests = \
  jb/clfft/ut_error \
  jb/clfft/ut_plan
jb_clfft_benchmarks = \
  jb/clfft/bm_single_fft
jb_ut_clfft_ldadd = \
  jb/libjb_clfft.a \
  $(CLFFT_LIB) \
  $(jb_ut_opencl_ldadd)
jb_ut_clfft_ldflags = \
  $(CLFFT_LDFLAGS) \
  $(jb_ut_opencl_ldflags)

lib_LIBRARIES += jb/libjb_tde.a
jb_tde_unit_tests = \
  jb/tde/ut_conjugate_and_multiply \
  jb/tde/ut_reduce_argmax_real
jb_tde_benchmarks = \
  jb/tde/bm_reduce_argmax_real
jb_ut_tde_ldadd = \
  jb/libjb_tde.a \
  $(jb_ut_clfft_ldadd)
jb_ut_tde_ldflags = \
  $(jb_tde_ldflags) \
  $(jb_ut_clfft_ldflags)

AM_CXXFLAGS += \
  $(OPENCL_CPPFLAGS) -D__CL_ENABLE_EXCEPTIONS \
  $(CLFFT_CPPFLAGS) $(BOOST_COMPUTE_CPPFLAGS) \
  $(BEAST_CPPFLAGS)

check_PROGRAMS += \
  $(jb_opencl_unit_tests) \
  $(jb_clfft_unit_tests) \
  $(jb_tde_unit_tests)

noinst_PROGRAMS += \
  $(jb_opencl_examples)

bin_PROGRAMS += \
  $(jb_opencl_benchmarks) \
  $(jb_clfft_benchmarks) \
  $(jb_tde_benchmarks)

#
# jb/libjb_opencl.a
#

jb_libjb_opencl_adir = $(includedir)
jb_libjb_opencl_a_SOURCES = \
  jb/opencl/build_simple_kernel.cpp \
  jb/opencl/config.cpp \
  jb/opencl/device_selector.cpp \
  jb/opencl/generic_reduce_program.cpp
nobase_jb_libjb_opencl_a_HEADERS = \
  jb/opencl/build_simple_kernel.hpp \
  jb/opencl/config.hpp \
  jb/opencl/device_selector.hpp \
  jb/opencl/generic_reduce_program.hpp

jb/opencl/generic_reduce_program.cpp: jb/opencl/generic_reduce_program.cl Makefile
	$(AM_V_GEN)echo '#include <jb/opencl/generic_reduce_program.hpp>' >$@; \
	echo '' >>$@; \
	echo 'char const jb::opencl::generic_reduce_program_source[] = R"""(' >>$@; \
	cat $< >>$@; \
	echo ')""";' >> $@


# Unit tests for jb/libjb_opencl.a

jb_opencl_ut_build_simple_kernel_SOURCES = jb/opencl/ut_build_simple_kernel.cpp
jb_opencl_ut_build_simple_kernel_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_opencl_ut_build_simple_kernel
jb_opencl_ut_build_simple_kernel_LDFLAGS = $(jb_opencl_ldflags)
jb_opencl_ut_build_simple_kernel_LDADD = $(jb_ut_opencl_ldadd)

jb_opencl_ut_config_SOURCES = jb/opencl/ut_config.cpp
jb_opencl_ut_config_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_opencl_ut_config
jb_opencl_ut_config_LDFLAGS = $(jb_opencl_ldflags)
jb_opencl_ut_config_LDADD = $(jb_ut_opencl_ldadd)

jb_opencl_ut_device_selector_SOURCES = jb/opencl/ut_device_selector.cpp
jb_opencl_ut_device_selector_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_opencl_ut_device_selector
jb_opencl_ut_device_selector_LDFLAGS = $(jb_opencl_ldflags)
jb_opencl_ut_device_selector_LDADD = $(jb_ut_opencl_ldadd)

jb_opencl_ut_generic_reduce_SOURCES = jb/opencl/ut_generic_reduce.cpp
jb_opencl_ut_generic_reduce_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_opencl_ut_generic_reduce
jb_opencl_ut_generic_reduce_LDFLAGS = $(jb_ut_opencl_ldflags)
jb_opencl_ut_generic_reduce_LDADD = $(jb_ut_opencl_ldadd)

jb_opencl_show_device_SOURCES = jb/opencl/show_device.cpp
jb_opencl_show_device_LDADD = jb/libjb_opencl.a jb/libjb.a $(OPENCL_LIB)

jb_opencl_bm_copy_buffer_SOURCES = jb/opencl/bm_copy_buffer.cpp
jb_opencl_bm_copy_buffer_LDFLAGS = $(jb_opencl_ldflags)
jb_opencl_bm_copy_buffer_LDADD = \
  jb/libjb_testing.a \
  jb/libjb_opencl_testing.a \
  jb/libjb_opencl.a \
  jb/libjb.a \
  $(OPENCL_LIB)

jb_opencl_bm_generic_reduce_SOURCES = jb/opencl/bm_generic_reduce.cpp
jb_opencl_bm_generic_reduce_LDFLAGS = $(jb_opencl_ldflags)
jb_opencl_bm_generic_reduce_LDADD = \
  jb/libjb_testing.a \
  jb/libjb_opencl_testing.a \
  jb/libjb_opencl.a \
  jb/libjb.a \
  $(OPENCL_LIB)

jb_opencl_bm_launch_kernel_SOURCES = jb/opencl/bm_launch_kernel.cpp
jb_opencl_bm_launch_kernel_LDFLAGS =   $(jb_opencl_ldflags)
jb_opencl_bm_launch_kernel_LDADD = \
  jb/libjb_testing.a \
  jb/libjb_opencl_testing.a \
  jb/libjb_opencl.a \
  jb/libjb.a \
  $(OPENCL_LIB)

#
# jb/libjb_opencl_testing.a
#
jb_libjb_opencl_testing_adir = $(includedir)
jb_libjb_opencl_testing_a_SOURCES = \
  jb/opencl/microbenchmark_config.cpp
nobase_jb_libjb_opencl_testing_a_HEADERS = \
  jb/opencl/microbenchmark_config.hpp

#
# jb/libjb_clfft.a
#

jb_libjb_clfft_adir = $(includedir)
jb_libjb_clfft_a_SOURCES = \
  jb/clfft/error.cpp \
  jb/clfft/init.cpp
nobase_jb_libjb_clfft_a_HEADERS = \
  jb/clfft/error.hpp \
  jb/clfft/init.hpp \
  jb/clfft/plan.hpp

jb_clfft_ut_error_SOURCES = jb/clfft/ut_error.cpp
jb_clfft_ut_error_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_clfft_ut_error
jb_clfft_ut_error_LDFLAGS = $(jb_ut_clfft_ldflags)
jb_clfft_ut_error_LDADD = $(jb_ut_clfft_ldadd)

jb_clfft_ut_plan_SOURCES = jb/clfft/ut_plan.cpp
jb_clfft_ut_plan_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_clfft_ut_plan
jb_clfft_ut_plan_LDFLAGS = $(jb_ut_clfft_ldflags)
jb_clfft_ut_plan_LDADD = $(jb_ut_clfft_ldadd)

jb_clfft_bm_single_fft_SOURCES = jb/clfft/bm_single_fft.cpp
jb_clfft_bm_single_fft_CPPFLAGS =
jb_clfft_bm_single_fft_LDFLAGS = \
  $(CLFFT_LDFLAGS) \
  $(jb_opencl_ldflags)
jb_clfft_bm_single_fft_LDADD = \
  jb/libjb_testing.a \
  jb/libjb_clfft.a \
  jb/libjb_opencl_testing.a \
  jb/libjb_opencl.a \
  jb/libjb.a \
  $(CLFFT_LIB) \
  $(OPENCL_LIB)

#
# jb/libjb_tde.a
#

jb_libjb_tde_a_SOURCES += \
  jb/tde/conjugate_and_multiply_kernel.cpp
nobase_jb_libjb_tde_a_HEADERS += \
  jb/tde/conjugate_and_multiply_kernel.hpp

jb/tde/conjugate_and_multiply_kernel.cpp: jb/tde/conjugate_and_multiply_kernel.cl Makefile
	$(AM_V_GEN)echo '#include <jb/tde/conjugate_and_multiply_kernel.hpp>' >$@; \
	echo '' >>$@; \
	echo 'char const jb::tde::conjugate_and_multiply_kernel_source[] = R"""(' >>$@; \
	cat $< >>$@; \
	echo ')""";' >> $@

jb/tde/reduce_argmax_real_kernel.cpp: jb/tde/reduce_argmax_real_kernel.cl Makefile
	$(AM_V_GEN)echo '#include <jb/tde/reduce_argmax_real_kernel.hpp>' >$@; \
	echo '' >>$@; \
	echo 'char const jb::tde::reduce_argmax_real_kernel_source[] = R"""(' >>$@; \
	cat $< >>$@; \
	echo ')""";' >> $@

jb_tde_ut_conjugate_and_multiply_SOURCES = jb/tde/ut_conjugate_and_multiply.cpp
jb_tde_ut_conjugate_and_multiply_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_tde_ut_conjugate_and_multiply
jb_tde_ut_conjugate_and_multiply_LDFLAGS = $(jb_ut_tde_ldflags)
jb_tde_ut_conjugate_and_multiply_LDADD = $(jb_ut_tde_ldadd)

jb_tde_ut_reduce_argmax_real_SOURCES = jb/tde/ut_reduce_argmax_real.cpp
jb_tde_ut_reduce_argmax_real_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_tde_ut_reduce_argmax_real
jb_tde_ut_reduce_argmax_real_LDFLAGS = $(jb_ut_tde_ldflags)
jb_tde_ut_reduce_argmax_real_LDADD = $(jb_ut_tde_ldadd)

jb_tde_bm_reduce_argmax_real_SOURCES = jb/tde/bm_reduce_argmax_real.cpp
jb_tde_bm_reduce_argmax_real_CPPFLAGS =
jb_tde_bm_reduce_argmax_real_LDADD = \
  jb/libjb_testing.a \
  jb/libjb_tde.a \
  jb/libjb_clfft.a \
  jb/libjb_opencl_testing.a \
  jb/libjb_opencl.a \
  jb/libjb.a \
  $(CLFFT_LIB) \
  $(OPENCL_LIB)
jb_tde_bm_reduce_argmax_real_LDFLAGS = \
  $(CLFFT_LDFLAGS) \
  $(OPENCL_LDFLAGS)
else
endif
