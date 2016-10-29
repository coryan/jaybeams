# JayBeams uses a non-recursive make, this is the only Makefile.

instdir = $(prefix)/lib
inst_LIBRARIES = \
  jb/libjb.a \
  jb/libitch5.a

noinst_LIBRARIES = \
  jb/libjb_testing.a \
  jb/libitch5_testing.a

jb_unit_tests = \
  jb/ut_as_hhmmss \
  jb/ut_assert_throw \
  jb/ut_book_depth_statistics \
  jb/ut_config_files_location \
  jb/ut_config_object \
  jb/ut_config_object_vector \
  jb/ut_event_rate_estimator \
  jb/ut_event_rate_histogram \
  jb/ut_explicit_cuts_binning \
  jb/ut_fileio \
  jb/ut_filetype \
  jb/ut_histogram \
  jb/ut_histogram_summary \
  jb/ut_integer_range_binning \
  jb/ut_logging \
  jb/ut_merge_yaml \
  jb/ut_offline_feed_statistics \
  jb/ut_severity_level \
  jb/ut_strtonum

jb_itch5_unit_tests = \
  jb/itch5/ut_add_order_message \
  jb/itch5/ut_add_order_mpid_message \
  jb/itch5/ut_base_decoders \
  jb/itch5/ut_base_encoders \
  jb/itch5/ut_broken_trade_message \
  jb/itch5/ut_char_list_field \
  jb/itch5/ut_char_list_validator \
  jb/itch5/ut_check_offset \
  jb/itch5/ut_compute_book_depth \
  jb/itch5/ut_compute_inside \
  jb/itch5/ut_cross_trade_message \
  jb/itch5/ut_cross_type \
  jb/itch5/ut_ipo_quoting_period_update_message \
  jb/itch5/ut_market_participant_position_message \
  jb/itch5/ut_message_header \
  jb/itch5/ut_mold_udp_pacer \
  jb/itch5/ut_mold_udp_pacer_config \
  jb/itch5/ut_mwcb_breach_message \
  jb/itch5/ut_mwcb_decline_level_message \
  jb/itch5/ut_net_order_imbalance_indicator_message \
  jb/itch5/ut_order_book \
  jb/itch5/ut_order_cancel_message \
  jb/itch5/ut_order_delete_message \
  jb/itch5/ut_order_executed_message \
  jb/itch5/ut_order_executed_price_message \
  jb/itch5/ut_order_replace_message \
  jb/itch5/ut_p2ceil \
  jb/itch5/ut_price_field \
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
  jb/itch5/ut_trade_message

jb_testing_unit_tests = \
  jb/testing/ut_create_square_timeseries \
  jb/testing/ut_create_triangle_timeseries \
  jb/testing/ut_delay_timeseries \
  jb/testing/ut_microbenchmark_config

unit_tests = $(jb_unit_tests) $(jb_testing_unit_tests) \
  $(jb_itch5_unit_tests)

benchmarks = \
  jb/bm_clocks

examples = \
  examples/configuration \
  tools/itch5bookdepth \
  tools/itch5inside \
  tools/itch5moldreplay \
  tools/itch5stats

noinst_PROGRAMS = $(examples) $(benchmarks)

check_PROGRAMS = $(unit_tests)
TESTS = $(check_PROGRAMS)

AM_CXXFLAGS = \
  -DJB_SYSCONFDIR=\"$(sysconfdir)\" \
  -DJB_BINDIR=\"$(bindir)\" \
  $(YAML_CPP_CPPFLAGS) \
  $(BOOST_CPPFLAGS) \
  -DBOOST_LOG_DYN_LINK \
  $(PTHREAD_CFLAGS)

LDFLAGS = \
  $(YAML_CPP_LDFLAGS) \
  $(BOOST_LDFLAGS)

YAML_CPP_LIBS=@YAML_CPP_LIBS@

LIBS = \
  $(YAML_CPP_LIBS) \
  $(BOOST_LOG_SETUP_LIB) \
  $(BOOST_LOG_LIB) \
  $(BOOST_FILESYSTEM_LIB) \
  $(BOOST_SYSTEM_LIB) \
  $(BOOST_DATE_TIME_LIB) \
  $(BOOST_THREAD_LIB) \
  $(BOOST_PROGRAM_OPTIONS_LIB) \
  $(BOOST_IOSTREAMS_LIB) \
  $(PTHREAD_LIBS)

# Common configuration for all unit tests
UT_CPPFLAGS = \
  -DBOOST_TEST_DYN_LINK \
  -DBOOST_TEST_MAIN \
  -DSKYE_USE_BOOST_UNIT_TEST_FRAMEWORK \
  $(CPPFLAGS)

jb_ut_libs = \
  jb/libjb.a \
  $(BOOST_UNIT_TEST_FRAMEWORK_LIB)

jb_ut_testing_libs = \
  jb/libjb_testing.a \
  $(jb_ut_libs)

jb_ut_itch5_libs = \
  jb/libitch5_testing.a \
  jb/libitch5.a \
  $(jb_ut_libs)

################################################################
# 
################################################################
check-style:
	$(AM_V_at)find $(top_srcdir) -name '*.[hic]pp' | \
	  xargs echo clang-format -i
	$(AM_V_at)git diff --exit-code
.PHONY: check-style

################################################################
# Optional FFTW stuff
################################################################

if USE_FFTW3
inst_LIBRARIES += jb/libjb_fftw.a
unit_tests += jb/fftw/ut_allocator \
  jb/fftw/ut_plan \
  jb/fftw/ut_time_delay_estimator \
  jb/fftw/ut_traits
benchmarks += jb/fftw/bm_time_delay_estimator
examples += examples/fftw_correlation_no_wrappers
AM_CXXFLAGS += $(FFTW3_CPPFLAGS) \
  $(FFTW3F_CPPFLAGS) \
  $(FFTW3L_CPPFLAGS)
LDFLAGS += $(FFTW3_LDFLAGS) \
  $(FFTW3F_LDFLAGS) \
  $(FFTW3L_LDFLAGS)
jb_ut_fftw_libs = \
  $(FFTW3_LIBS) \
  $(FFTW3F_LIBS) \
  $(FFTW3L_LIBS) \
  jb/libjb_testing.a \
  jb/libjb_fftw.a \
  $(jb_ut_libs)
else
jb_ut_fftw_libs =
endif !USE_FFTW3



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
  jb/convert_severity_level.hpp \
  jb/event_rate_estimator.hpp \
  jb/event_rate_histogram.hpp \
  jb/explicit_cuts_binning.hpp \
  jb/feed_error.hpp \
  jb/fileio.hpp \
  jb/filetype.hpp \
  jb/histogram.hpp \
  jb/histogram_summary.hpp \
  jb/integer_range_binning.hpp \
  jb/log.hpp \
  jb/merge_yaml.hpp \
  jb/offline_feed_statistics.hpp \
  jb/severity_level.hpp \
  jb/strtonum.hpp \
  jb/usage.hpp

jb_libjb_a_SOURCES = \
  jb/as_hhmmss.cpp \
  jb/assert_throw.cpp \
  jb/book_depth_statistics.cpp \
  jb/config_files_location.cpp \
  jb/config_object.cpp \
  jb/fileio.cpp \
  jb/filetype.cpp \
  jb/histogram_summary.cpp \
  jb/log.cpp \
  jb/merge_yaml.cpp \
  jb/offline_feed_statistics.cpp \
  jb/severity_level.cpp

jb_libjb_a_LIBADD = 

################################################################
# jb/libjb_testing.a
################################################################

jb_libjb_testing_adir = $(includedir)
jb_libjb_testing_a_SOURCES = \
  jb/testing/compile_info.cpp \
  jb/testing/microbenchmark_base.cpp \
  jb/testing/microbenchmark_config.cpp

jb_libjb_testing_a_HEADERS = \
  jb/testing/check_array_close_enough.hpp \
  jb/testing/check_complex_close_enough.hpp \
  jb/testing/check_vector_close_enough.hpp \
  jb/testing/create_square_timeseries.hpp \
  jb/testing/create_triangle_timeseries.hpp \
  jb/testing/delay_timeseries.hpp \
  jb/testing/microbenchmark.hpp \
  jb/testing/microbenchmark_base.hpp \
  jb/testing/microbenchmark_config.hpp

jb/testing/compile_info.cpp: jb/testing/compile_info.hpp
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
	echo $(CXXFLAGS) >>$@; \
	echo ')""";' >> $@; \
	echo '' >>$@; \
	echo 'char const jb::testing::linker[] = R"""(' >>$@; \
	$(CXX) -Wl,--version >>$@; \
	echo ')""";' >> $@; \
	echo '' >>$@

################################################################
# jb/libitch5_testing.a
################################################################

jb_libitch5_testing_adir = $(includedir)
jb_libitch5_testing_a_SOURCES = \
  jb/itch5/testing_data.cpp
jb_libitch5_testing_a_HEADERS = \
  jb/itch5/testing_data.hpp

################################################################
# jb/libitch5.a
################################################################

jb_libitch5_adir = $(includedir)
nobase_jb_libitch5_a_HEADERS =

jb_libitch5_a_SOURCES = \
  jb/itch5/add_order_message.cpp \
  jb/itch5/add_order_mpid_message.cpp \
  jb/itch5/base_decoders.cpp \
  jb/itch5/broken_trade_message.cpp \
  jb/itch5/char_list_validator.cpp \
  jb/itch5/check_offset.cpp \
  jb/itch5/compute_book_depth.cpp \
  jb/itch5/compute_inside.cpp \
  jb/itch5/cross_trade_message.cpp \
  jb/itch5/ipo_quoting_period_update_message.cpp \
  jb/itch5/market_participant_position_message.cpp \
  jb/itch5/message_header.cpp \
  jb/itch5/mold_udp_pacer_config.cpp \
  jb/itch5/mwcb_breach_message.cpp \
  jb/itch5/mwcb_decline_level_message.cpp \
  jb/itch5/net_order_imbalance_indicator_message.cpp \
  jb/itch5/order_book.cpp \
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
  jb/itch5/trade_message.cpp

jb_libitch5_a_HEADERS = \
  jb/itch5/add_order_message.hpp \
  jb/itch5/add_order_mpid_message.hpp \
  jb/itch5/base_decoders.hpp \
  jb/itch5/base_encoders.hpp \
  jb/itch5/broken_trade_message.hpp \
  jb/itch5/char_list_field.hpp \
  jb/itch5/char_list_validator.hpp \
  jb/itch5/check_offset.hpp \
  jb/itch5/compute_book_depth.hpp \
  jb/itch5/compute_inside.hpp \
  jb/itch5/cross_trade_message.hpp \
  jb/itch5/cross_type.hpp \
  jb/itch5/decoder.hpp \
  jb/itch5/ipo_quoting_period_update_message.hpp \
  jb/itch5/market_participant_position_message.hpp \
  jb/itch5/message_header.hpp \
  jb/itch5/mold_udp_pacer.hpp \
  jb/itch5/mold_udp_pacer_config.hpp \
  jb/itch5/mold_udp_protocol_constants.hpp \
  jb/itch5/mpid_field.hpp \
  jb/itch5/mwcb_breach_message.hpp \
  jb/itch5/mwcb_decline_level_message.hpp \
  jb/itch5/net_order_imbalance_indicator_message.hpp \
  jb/itch5/order_book.hpp \
  jb/itch5/order_book_def.hpp \
  jb/itch5/order_cancel_message.hpp \
  jb/itch5/order_delete_message.hpp \
  jb/itch5/order_executed_message.hpp \
  jb/itch5/order_executed_price_message.hpp \
  jb/itch5/order_replace_message.hpp \
  jb/itch5/p2ceil.hpp \
  jb/itch5/price_field.hpp \
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
  jb/itch5/unknown_message.hpp

jb_libitch5_a_LIBADD =

################################################################
# jb/libjb_fftw.a
################################################################

jb_libjb_fftw_adir = $(includedir)
jb_libjb_fftw_a_SOURCES =
jb_libjb_fftw_a_HEADERS = \
  jb/fftw/aligned_vector.hpp \
  jb/fftw/allocator.hpp \
  jb/fftw/cast.hpp \
  jb/fftw/plan.hpp \
  jb/fftw/time_delay_estimator.hpp \
  jb/fftw/traits.hpp

################################################################
# examples
################################################################

examples_configuration_SOURCES = examples/configuration.cpp
examples_configuration_LDADD = jb/libjb.a

examples_fftw_correlation_no_wrappers_SOURCES = examples/fftw_correlation_no_wrappers.cpp
examples_fftw_correlation_no_wrappers_LDADD = jb/libjb.a $(FFTW3_LIBS)

tools_itch5bookdepth_SOURCES = tools/itch5bookdepth.cpp
tools_itch5bookdepth_LDADD = jb/libitch5.a jb/libjb.a

tools_itch5inside_SOURCES = tools/itch5inside.cpp
tools_itch5inside_LDADD = jb/libitch5.a jb/libjb.a

tools_itch5moldreplay_SOURCES = tools/itch5moldreplay.cpp
tools_itch5moldreplay_LDADD = jb/libitch5.a jb/libjb.a $(BOOST_ASIO_LIB)

tools_itch5stats_SOURCES = tools/itch5stats.cpp
tools_itch5stats_LDADD = jb/libitch5.a jb/libjb.a

################################################################
# benchmarks and unit tests
################################################################

jb_bm_clocks_SOURCES = jb/bm_clocks.cpp
jb_bm_clocks_CPPFLAGS =
jb_bm_clocks_LDADD = jb/libjb_testing.a jb/libjb.a

jb_ut_as_hhmmss_SOURCES = jb/ut_as_hhmmss.cpp
jb_ut_as_hhmmss_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_as_hhmmss
jb_ut_as_hhmmss_LDADD = $(jb_ut_libs)

jb_ut_assert_throw_SOURCES = jb/ut_assert_throw.cpp
jb_ut_assert_throw_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_assert_throw
jb_ut_assert_throw_LDADD = $(jb_ut_libs)

jb_ut_book_depth_statistics_SOURCES = jb/ut_book_depth_statistics.cpp
jb_ut_book_depth_statistics_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_book_depth_statistics
jb_ut_book_depth_statistics_LDADD = $(jb_ut_libs)

jb_ut_config_files_location_SOURCES = jb/ut_config_files_location.cpp
jb_ut_config_files_location_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_config_files_location
jb_ut_config_files_location_LDADD = $(jb_ut_libs)

jb_ut_config_object_SOURCES = jb/ut_config_object.cpp
jb_ut_config_object_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_config_object
jb_ut_config_object_LDADD = $(jb_ut_libs)

jb_ut_config_object_vector_SOURCES = jb/ut_config_object_vector.cpp
jb_ut_config_object_vector_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_config_object_vector
jb_ut_config_object_vector_LDADD = $(jb_ut_libs)

jb_ut_event_rate_estimator_SOURCES = jb/ut_event_rate_estimator.cpp
jb_ut_event_rate_estimator_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_event_rate_estimator
jb_ut_event_rate_estimator_LDADD = $(jb_ut_libs)

jb_ut_event_rate_histogram_SOURCES = jb/ut_event_rate_histogram.cpp
jb_ut_event_rate_histogram_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_event_rate_histogram
jb_ut_event_rate_histogram_LDADD = $(jb_ut_libs)

jb_ut_explicit_cuts_binning_SOURCES = jb/ut_explicit_cuts_binning.cpp
jb_ut_explicit_cuts_binning_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_explicit_cuts_binning
jb_ut_explicit_cuts_binning_LDADD = $(jb_ut_libs)

jb_ut_fileio_SOURCES = jb/ut_fileio.cpp
jb_ut_fileio_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_fileio
jb_ut_fileio_LDADD = $(jb_ut_libs)

jb_ut_filetype_SOURCES = jb/ut_filetype.cpp
jb_ut_filetype_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_filetype
jb_ut_filetype_LDADD = $(jb_ut_libs)

jb_ut_histogram_SOURCES = jb/ut_histogram.cpp
jb_ut_histogram_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_histogram
jb_ut_histogram_LDADD = $(jb_ut_libs)

jb_ut_histogram_summary_SOURCES = jb/ut_histogram.cpp
jb_ut_histogram_summary_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_histogram_summary
jb_ut_histogram_summary_LDADD = $(jb_ut_libs)

jb_ut_integer_range_binning_SOURCES = jb/ut_integer_range_binning.cpp
jb_ut_integer_range_binning_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_integer_range_binning
jb_ut_integer_range_binning_LDADD = $(jb_ut_libs)

jb_ut_logging_SOURCES = jb/ut_logging.cpp
jb_ut_logging_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_logging
jb_ut_logging_LDADD = $(jb_ut_libs)

jb_ut_merge_yaml_SOURCES = jb/ut_merge_yaml.cpp
jb_ut_merge_yaml_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_merge_yaml
jb_ut_merge_yaml_LDADD = $(jb_ut_libs)

jb_ut_offline_feed_statistics_SOURCES = jb/ut_offline_feed_statistics.cpp
jb_ut_offline_feed_statistics_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_offline_feed_statistics
jb_ut_offline_feed_statistics_LDADD = $(jb_ut_libs)

jb_ut_severity_level_SOURCES = jb/ut_severity_level.cpp
jb_ut_severity_level_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_severity_level
jb_ut_severity_level_LDADD = $(jb_ut_libs)

jb_ut_strtonum_SOURCES = jb/ut_strtonum.cpp
jb_ut_strtonum_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_ut_strtonum
jb_ut_strtonum_LDADD = $(jb_ut_libs)

################################################################
# fftw unit tests
################################################################

jb_fftw_ut_allocator_SOURCES = jb/fftw/ut_allocator.cpp
jb_fftw_ut_allocator_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_fftw_ut_allocator
jb_fftw_ut_allocator_LDADD = $(jb_ut_fftw_libs)

jb_fftw_ut_plan_SOURCES = jb/fftw/ut_plan.cpp
jb_fftw_ut_plan_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_fftw_ut_plan
jb_fftw_ut_plan_LDADD = $(jb_ut_fftw_libs)

jb_fftw_ut_time_delay_estimator_SOURCES = jb/fftw/ut_time_delay_estimator.cpp
jb_fftw_ut_time_delay_estimator_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_fftw_ut_time_delay_estimator
jb_fftw_ut_time_delay_estimator_LDADD = $(jb_ut_fftw_libs)

jb_fftw_ut_traits_SOURCES = jb/fftw/ut_traits.cpp
jb_fftw_ut_traits_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_fftw_ut_traits
jb_fftw_ut_traits_LDADD = $(jb_ut_fftw_libs)

################################################################
# fftw benchmarks
################################################################

jb_fftw_bm_time_delay_estimator_SOURCES = jb/fftw/bm_time_delay_estimator.cpp
jb_fftw_bm_time_delay_estimator_CPPFLAGS =
jb_fftw_bm_time_delay_estimator_LDADD = \
  $(FFTW3_LIBS) \
  $(FFTW3F_LIBS) \
  $(FFTW3L_LIBS) \
  jb/libjb_testing.a \
  jb/libjb_fftw.a \
  jb/libjb.a

################################################################
# itch5 unit tests
################################################################

jb_itch5_ut_add_order_message_SOURCES = jb/itch5/ut_add_order_message.cpp
jb_itch5_ut_add_order_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_add_order_message
jb_itch5_ut_add_order_message_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_add_order_mpid_message_SOURCES = jb/itch5/ut_add_order_mpid_message.cpp
jb_itch5_ut_add_order_mpid_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_add_order_mpid_messag
jb_itch5_ut_add_order_mpid_message_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_base_decoders_SOURCES = jb/itch5/ut_base_decoders.cpp
jb_itch5_ut_base_decoders_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_base_decoders
jb_itch5_ut_base_decoders_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_base_encoders_SOURCES = jb/itch5/ut_base_encoders.cpp
jb_itch5_ut_base_encoders_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_base_encoders
jb_itch5_ut_base_encoders_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_broken_trade_message_SOURCES = jb/itch5/ut_broken_trade_message.cpp
jb_itch5_ut_broken_trade_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_broken_trade_message
jb_itch5_ut_broken_trade_message_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_char_list_field_SOURCES = jb/itch5/ut_char_list_field.cpp
jb_itch5_ut_char_list_field_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_char_list_field
jb_itch5_ut_char_list_field_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_char_list_validator_SOURCES = jb/itch5/ut_char_list_validator.cpp
jb_itch5_ut_char_list_validator_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_char_list_validator
jb_itch5_ut_char_list_validator_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_check_offset_SOURCES = jb/itch5/ut_check_offset.cpp
jb_itch5_ut_check_offset_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_check_offset
jb_itch5_ut_check_offset_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_compute_book_depth_SOURCES = jb/itch5/ut_compute_book_depth.cpp
jb_itch5_ut_compute_book_depth_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_compute_book_depth
jb_itch5_ut_compute_book_depth_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_compute_inside_SOURCES = jb/itch5/ut_compute_inside.cpp
jb_itch5_ut_compute_inside_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_compute_inside
jb_itch5_ut_compute_inside_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_cross_trade_message_SOURCES = jb/itch5/ut_cross_trade_message.cpp
jb_itch5_ut_cross_trade_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_cross_trade_message
jb_itch5_ut_cross_trade_message_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_cross_type_SOURCES = jb/itch5/ut_cross_type.cpp
jb_itch5_ut_cross_type_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_cross_type
jb_itch5_ut_cross_type_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_ipo_quoting_period_update_message_SOURCES = jb/itch5/ut_ipo_quoting_period_update_message.cpp
jb_itch5_ut_ipo_quoting_period_update_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_ipo_quoting_period_update_message
jb_itch5_ut_ipo_quoting_period_update_message_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_market_participant_position_message_SOURCES = jb/itch5/ut_market_participant_position_message.cpp
jb_itch5_ut_market_participant_position_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_market_participant_position_message
jb_itch5_ut_market_participant_position_message_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_message_header_SOURCES = jb/itch5/ut_message_header.cpp
jb_itch5_ut_message_header_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_message_header
jb_itch5_ut_message_header_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_mold_udp_pacer_SOURCES = jb/itch5/ut_mold_udp_pacer.cpp
jb_itch5_ut_mold_udp_pacer_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_mold_udp_pacer
jb_itch5_ut_mold_udp_pacer_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_mold_udp_pacer_config_SOURCES = jb/itch5/ut_mold_udp_pacer_config.cpp
jb_itch5_ut_mold_udp_pacer_config_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_mold_udp_pacer_config
jb_itch5_ut_mold_udp_pacer_config_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_mwcb_breach_message_SOURCES = jb/itch5/ut_mwcb_breach_message.cpp
jb_itch5_ut_mwcb_breach_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_mwcb_breach_message
jb_itch5_ut_mwcb_breach_message_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_mwcb_decline_level_message_SOURCES = jb/itch5/ut_mwcb_decline_level_message.cpp
jb_itch5_ut_mwcb_decline_level_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_mwcb_decline_level_message
jb_itch5_ut_mwcb_decline_level_message_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_net_order_imbalance_indicator_message_SOURCES = jb/itch5/ut_net_order_imbalance_indicator_message.cpp
jb_itch5_ut_net_order_imbalance_indicator_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_net_order_imbalance_indicator_message
jb_itch5_ut_net_order_imbalance_indicator_message_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_order_book_SOURCES = jb/itch5/ut_order_book.cpp
jb_itch5_ut_order_book_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_order_book
jb_itch5_ut_order_book_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_order_cancel_message_SOURCES = jb/itch5/ut_order_cancel_message.cpp
jb_itch5_ut_order_cancel_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_order_cancel_message
jb_itch5_ut_order_cancel_message_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_order_delete_message_SOURCES = jb/itch5/ut_order_delete_message.cpp
jb_itch5_ut_order_delete_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_order_delete_message
jb_itch5_ut_order_delete_message_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_order_executed_message_SOURCES = jb/itch5/ut_order_executed_message.cpp
jb_itch5_ut_order_executed_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_order_executed_message
jb_itch5_ut_order_executed_message_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_order_executed_price_message_SOURCES = jb/itch5/ut_order_executed_price_message.cpp
jb_itch5_ut_order_executed_price_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_order_executed_price_message
jb_itch5_ut_order_executed_price_message_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_order_replace_message_SOURCES = jb/itch5/ut_order_replace_message.cpp
jb_itch5_ut_order_replace_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_order_replace_message
jb_itch5_ut_order_replace_message_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_p2ceil_SOURCES = jb/itch5/ut_p2ceil.cpp
jb_itch5_ut_p2ceil_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_p2ceil
jb_itch5_ut_p2ceil_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_price_field_SOURCES = jb/itch5/ut_price_field.cpp
jb_itch5_ut_price_field_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_price_field
jb_itch5_ut_price_field_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_process_buffer_mlist_SOURCES = jb/itch5/ut_process_buffer_mlist.cpp
jb_itch5_ut_process_buffer_mlist_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_process_buffer_mlist
jb_itch5_ut_process_buffer_mlist_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_process_iostream_mlist_SOURCES = jb/itch5/ut_process_iostream_mlist.cpp
jb_itch5_ut_process_iostream_mlist_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_process_iostream_mlist
jb_itch5_ut_process_iostream_mlist_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_reg_sho_restriction_message_SOURCES = jb/itch5/ut_reg_sho_restriction_message.cpp
jb_itch5_ut_reg_sho_restriction_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_reg_sho_restriction_message
jb_itch5_ut_reg_sho_restriction_message_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_seconds_field_SOURCES = jb/itch5/ut_seconds_field.cpp
jb_itch5_ut_seconds_field_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_seconds_field
jb_itch5_ut_seconds_field_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_short_string_field_SOURCES = jb/itch5/ut_short_string_field.cpp
jb_itch5_ut_short_string_field_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_short_string_field
jb_itch5_ut_short_string_field_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_static_digits_SOURCES = jb/itch5/ut_static_digits.cpp
jb_itch5_ut_static_digits_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_static_digits
jb_itch5_ut_static_digits_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_stock_directory_message_SOURCES = jb/itch5/ut_stock_directory_message.cpp
jb_itch5_ut_stock_directory_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_stock_directory_message
jb_itch5_ut_stock_directory_message_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_stock_trading_action_message_SOURCES = jb/itch5/ut_stock_trading_action_message.cpp
jb_itch5_ut_stock_trading_action_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_stock_trading_action_message
jb_itch5_ut_stock_trading_action_message_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_system_event_message_SOURCES = jb/itch5/ut_system_event_message.cpp
jb_itch5_ut_system_event_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_system_event_message
jb_itch5_ut_system_event_message_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_timestamp_SOURCES = jb/itch5/ut_timestamp.cpp
jb_itch5_ut_timestamp_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_timestamp
jb_itch5_ut_timestamp_LDADD = $(jb_ut_itch5_libs)

jb_itch5_ut_trade_message_SOURCES = jb/itch5/ut_trade_message.cpp
jb_itch5_ut_trade_message_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_itch5_ut_trade_message
jb_itch5_ut_trade_message_LDADD = $(jb_ut_itch5_libs)

################################################################
# testing unit tests
################################################################

jb_testing_ut_create_square_timeseries_SOURCES = jb/testing/ut_create_square_timeseries.cpp
jb_testing_ut_create_square_timeseries_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_testing_ut_create_square_timeseries
jb_testing_ut_create_square_timeseries_LDADD = $(jb_ut_testing_libs)

jb_testing_ut_create_triangle_timeseries_SOURCES = jb/testing/ut_create_triangle_timeseries.cpp
jb_testing_ut_create_triangle_timeseries_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_testing_ut_create_triangle_timeseries
jb_testing_ut_create_triangle_timeseries_LDADD = $(jb_ut_testing_libs)

jb_testing_ut_delay_timeseries_SOURCES = jb/testing/ut_delay_timeseries.cpp
jb_testing_ut_delay_timeseries_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_testing_ut_delay_timeseries
jb_testing_ut_delay_timeseries_LDADD = $(jb_ut_testing_libs)

jb_testing_ut_microbenchmark_config_SOURCES = jb/testing/ut_microbenchmark_config.cpp
jb_testing_ut_microbenchmark_config_CPPFLAGS = $(UT_CPPFLAGS) -DBOOST_TEST_MODULE=jb_testing_ut_microbenchmark_config
jb_testing_ut_microbenchmark_config_LDADD = $(jb_ut_testing_libs)
