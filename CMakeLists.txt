#   Copyright 2017 Carlos O'Ryan
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#

cmake_minimum_required(VERSION 3.5)

set(PACKAGE_NAME "JayBeams")
set(PACKAGE_VERSION "0.1")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_TARNAME "${PACKAGE_NAME}-${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "https://github.com/coryan/jaybeams/issues")

project(JayBeams CXX C)

# Require C++14 for this project ...
set(CMAKE_CXX_STANDARD 14)
set(CXX_STANDARD_REQUIRED ON)

option(JB_DISABLE_TESTS "Disable unit and integration tests. Can be useful to avoid installing gmock+gtest." OFF)

# ... the author is paranoid.  Turn on all available warnings
# and turn warnings into errors to stop the build if any warning is
# emitted ...
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG(-Werror COMPILER_SUPPORTS_WERROR)
if (COMPILER_SUPPORTS_WERROR)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif ()
CHECK_CXX_COMPILER_FLAG(-Wall COMPILER_SUPPORTS_WALL)
if (COMPILER_SUPPORTS_WALL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif ()
CHECK_CXX_COMPILER_FLAG(/WX COMPILER_SUPPORTS_WX)
if (COMPILER_SUPPORTS_WX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
endif ()
CHECK_CXX_COMPILER_FLAG(/W4 COMPILER_SUPPORTS_SWALL)
if (COMPILER_SUPPORTS_SWALL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Wall")
endif ()

# ... if set, enable code coverage builds for g++, and create a "test_coverage" target to collect the coverage using
# lcov.  My attempts at using the gcovr cobertura output failed miserably, so I am sticking to what I know ...
option(COVERAGE "Enable code coverage builds." OFF)
if (${COVERAGE})
    include(cmake/CodeCoverage.cmake)
    include(ProcessorCount)
    ProcessorCount(NCPUS)
    set(ctest_parallelism "")
    if (NOT N EQUAL 0)
        set(ctest_parallelism "-j ${NCPUS}")
    endif ()
    APPEND_COVERAGE_COMPILER_FLAGS()
    SETUP_TARGET_FOR_COVERAGE(
            NAME test_coverage
            EXECUTABLE ctest ${ctest_parallelism}
    )
endif (${COVERAGE})

include(cmake/FindSanitizers.cmake)
message(STATUS ${CMAKE_CXX_FLAGS})

# ... include the functions to compile proto files ...
include(FindProtobuf)

# ... find the protobuf, grpc, and grpc++ libraries using pkg-config ...
include(FindPkgConfig)
pkg_check_modules(PROTOBUF REQUIRED protobuf>=3.0)
pkg_check_modules(GRPCPP REQUIRED grpc++>=1.0)
pkg_check_modules(GRPC REQUIRED grpc>=4.0)
link_directories(${GRPCPP_LIBRARY_DIRS} ${GRPC_LIBRARY_DIRS} ${PROTOBUF_LIBRARY_DIRS})
include_directories(${GRPCPP_INCLUDE_DIRS} ${GRPC_INCLUDE_DIRS} ${PROTOBUF_INCLUDE_DIRS})

# ... we need Boost >= 1.58 because Beast and Boost.Endian require them ...
include(FindBoost)
find_package(Boost 1.58 QUIET REQUIRED
        COMPONENTS log log_setup filesystem date_time thread iostreams program_options unit_test_framework)
include_directories(${Boost_INCLUDE_DIRS})

# ... YAML-CPP is a library to parse YAML files, JayBeams uses such files for all runtime configuration ...
find_package(yaml-cpp QUIET)
if (yaml-cpp_FOUND)
    message(STATUS "Found yaml-cpp ${yaml-cpp_VERSION} using find_package()")
    set(YAMLCPP_LIBRARIES yaml-cpp)
else ()
    pkg_check_modules(YAMLCPP REQUIRED yaml-cpp>=0.5)
    message(STATUS "Found yaml-cpp using pkg_check_modules()")
endif ()

# ... Beast (https://github.com/boostorg/beast) is a library for writing HTTP clients and servers ...
option(Boost_USE_STATIC_LIBS "Disable static Boost libraries in Beast" OFF)
option(Beast_BUILD_EXAMPLES "Disable examples in Beast" OFF)
option(Beast_BUILD_TESTS "Disable tests in Beast" OFF)
add_subdirectory(ext/Beast)
include_directories(${PROJECT_SOURCE_DIR}/ext/Beast/include)

# ... we need the -I flags so we can find header files using the installed path for JayBeams headers ...
include_directories("${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}")

# Compile things into shared libraries, apparently it is easy to instruct cmake(1) to generate a static library even
# if we only describe how to create a shared lib.
add_library(jb SHARED
        jb/as_hhmmss.cpp
        jb/as_hhmmss.hpp
        jb/assert_throw.cpp
        jb/assert_throw.hpp
        jb/book_depth_statistics.cpp
        jb/book_depth_statistics.hpp
        jb/complex_traits.hpp
        jb/config_attribute.hpp
        jb/config_files_location.cpp
        jb/config_files_location.hpp
        jb/config_object.cpp
        jb/config_object.hpp
        jb/config_recurse.hpp
        jb/convert_cpu_set.hpp
        jb/convert_severity_level.hpp
        jb/cpu_set.cpp
        jb/cpu_set.hpp
        jb/detail/array_traits.hpp
        jb/detail/os_error.cpp
        jb/detail/os_error.hpp
        jb/detail/reconfigure_thread.cpp
        jb/detail/reconfigure_thread.hpp
        jb/detail/thread_setup_wrapper.hpp
        jb/event_rate_estimator.hpp
        jb/event_rate_histogram.hpp
        jb/explicit_cuts_binning.hpp
        jb/feed_error.hpp
        jb/fileio.cpp
        jb/fileio.hpp
        jb/filetype.cpp
        jb/filetype.hpp
        jb/fixed_string.hpp
        jb/histogram.hpp
        jb/histogram_summary.cpp
        jb/histogram_summary.hpp
        jb/integer_range_binning.hpp
        jb/launch_thread.hpp
        jb/log.cpp
        jb/log.hpp
        jb/merge_yaml.cpp
        jb/merge_yaml.hpp
        jb/offline_feed_statistics.cpp
        jb/offline_feed_statistics.hpp
        jb/p2ceil.hpp
        jb/severity_level.cpp
        jb/severity_level.hpp
        jb/strtonum.hpp
        jb/thread_config.cpp
        jb/thread_config.hpp
        jb/usage.hpp
        )
target_compile_definitions(jb PUBLIC -DBOOST_LOG_DYN_LINK)
target_link_libraries(jb Boost::log Boost::program_options Boost::iostreams ${YAMLCPP_LIBRARIES})
# ... the jb library unit tests, we setup the targets and dependencies further down using a loop ...
set(jb_unit_tests
        jb/ut_as_hhmmss
        jb/ut_assert_throw
        jb/ut_book_depth_statistics
        jb/ut_config_files_location
        jb/ut_config_object
        jb/ut_config_object_vector
        jb/ut_cpu_set
        jb/ut_event_rate_estimator
        jb/ut_event_rate_histogram
        jb/ut_explicit_cuts_binning
        jb/ut_fileio
        jb/ut_filetype
        jb/ut_fixed_string
        jb/ut_histogram
        jb/ut_histogram_summary
        jb/ut_integer_range_binning
        jb/ut_launch_thread
        jb/ut_logging
        jb/ut_merge_yaml
        jb/ut_offline_feed_statistics
        jb/ut_p2ceil
        jb/ut_severity_level
        jb/ut_strtonum
        jb/ut_thread_config
        )
add_executable(examples_configuration examples/configuration.cpp)
target_link_libraries(examples_configuration jb)

##
## Capture configuration information in a series of variables, useful in benchmarks and tests
##
if (WIN32)
    set(UNAME_LOG "TODO(coryan) - capture windows config")
else ()
    execute_process(COMMAND uname -a OUTPUT_VARIABLE UNAME_LOG ERROR_VARIABLE UNAME_LOG)
endif (WIN32)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CXX_VERSION "TODO(coryan) - capture compiler version for MSVC")
else ()
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} --version
            OUTPUT_VARIABLE CXX_VERSION_LOG ERROR_VARIABLE CXX_VERSION_LOG)
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CXXLD_VERSION "TODO(coryan) - capture linker version for MSVC")
else ()
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -Wl,--version
            OUTPUT_VARIABLE CXXLD_VERSION_LOG ERROR_VARIABLE CXXLD_VERSION_LOG)
endif ()

if (IS_DIRECTORY ${PROJECT_SOURCE_DIR}/.git)
    execute_process(COMMAND git rev-parse HEAD
            OUTPUT_VARIABLE GIT_HEAD_LOG ERROR_VARIABLE GIT_HEAD_LOG)
else ()
    set(GIT_HEAD_LOG "not .git directory")
endif ()

configure_file(jb/testing/compile_info.cpp.in jb/testing/compile_info.cpp)

add_library(jb_testing SHARED
        jb/testing/check_close_enough.hpp
        ${PROJECT_BINARY_DIR}/jb/testing/compile_info.cpp
        jb/testing/compile_info.hpp
        jb/testing/create_square_timeseries.hpp
        jb/testing/create_triangle_timeseries.hpp
        jb/testing/delay_timeseries.hpp
        jb/testing/future_status.hpp
        jb/testing/initialize_mersenne_twister.hpp
        jb/testing/microbenchmark.hpp
        jb/testing/microbenchmark_base.cpp
        jb/testing/microbenchmark_base.hpp
        jb/testing/microbenchmark_config.cpp
        jb/testing/microbenchmark_config.hpp
        jb/testing/microbenchmark_group_main.cpp
        jb/testing/microbenchmark_group_main.hpp
        jb/testing/sum_square.hpp
        )
target_link_libraries(jb_testing jb Boost::log Boost::program_options Boost::iostreams yaml-cpp)
set(jb_testing_unit_tests
        jb/testing/future_status_ut
        jb/testing/ut_check_close_enough
        jb/testing/ut_create_square_timeseries
        jb/testing/ut_create_triangle_timeseries
        jb/testing/ut_delay_timeseries
        jb/testing/ut_microbenchmark_config
        )
add_executable(jb_testing_check_mt19937_initializer jb/testing/check_mt19937_initializer.cpp)
target_link_libraries(jb_testing_check_mt19937_initializer jb_testing jb)

add_executable(jb_testing_check_random_device jb/testing/check_random_device.cpp)
target_link_libraries(jb_testing_check_random_device jb_testing jb)

add_executable(jb_bm_clocks jb/bm_clocks.cpp)
target_link_libraries(jb_bm_clocks jb_testing jb)

add_executable(jb_testing_show_compile_info jb/testing/show_compile_info.cpp)
target_link_libraries(jb_testing_show_compile_info jb_testing jb)

# ... jb_gmock needs the googletest libraries ...
add_subdirectory(ext/googletest/googlemock)
add_library(jb_gmock SHARED
        jb/gmock/init.hpp
        )
target_link_libraries(jb_gmock gmock gtest)

add_library(jb_ehs SHARED
        jb/ehs/acceptor.cpp
        jb/ehs/acceptor.hpp
        jb/ehs/base_types.hpp
        jb/ehs/connection.cpp
        jb/ehs/connection.hpp
        jb/ehs/request_dispatcher.cpp
        jb/ehs/request_dispatcher.hpp
        )
target_link_libraries(jb_ehs jb Boost::log Boost::program_options Boost::iostreams yaml-cpp)
set(jb_ehs_unit_tests
        jb/ehs/ut_acceptor
        jb/ehs/ut_request_dispatcher)

add_library(jb_pitch2 SHARED
        jb/pitch2/add_order_message.cpp
        jb/pitch2/add_order_message.hpp
        jb/pitch2/auction_update_message.cpp
        jb/pitch2/auction_update_message.hpp
        jb/pitch2/base_add_order_message.hpp
        jb/pitch2/base_add_order_message_streaming.hpp
        jb/pitch2/delete_order_message.cpp
        jb/pitch2/delete_order_message.hpp
        jb/pitch2/expanded_add_order_message.cpp
        jb/pitch2/expanded_add_order_message.hpp
        jb/pitch2/modify_message.cpp
        jb/pitch2/modify_message.hpp
        jb/pitch2/order_executed_message.cpp
        jb/pitch2/order_executed_message.hpp
        jb/pitch2/order_executed_price_message.cpp
        jb/pitch2/order_executed_price_message.hpp
        jb/pitch2/reduce_size_message.cpp
        jb/pitch2/reduce_size_message.hpp
        jb/pitch2/short_add_order_message.cpp
        jb/pitch2/short_add_order_message.hpp
        jb/pitch2/time_message.cpp
        jb/pitch2/time_message.hpp
        jb/pitch2/unit_clear_message.cpp
        jb/pitch2/unit_clear_message.hpp
        )
set(jb_pitch2_unit_tests
        jb/pitch2/ut_add_order_message
        jb/pitch2/ut_auction_update_message
        jb/pitch2/ut_delete_order_message
        jb/pitch2/ut_expanded_add_order_message
        jb/pitch2/ut_modify_message
        jb/pitch2/ut_order_executed_message
        jb/pitch2/ut_order_executed_price_message
        jb/pitch2/ut_reduce_size_message
        jb/pitch2/ut_short_add_order_message
        jb/pitch2/ut_time_message
        jb/pitch2/ut_unit_clear_message
        )
target_link_libraries(jb_pitch2 jb Boost::log Boost::program_options Boost::iostreams yaml-cpp)

add_library(jb_itch5 SHARED
        jb/itch5/add_order_message.cpp
        jb/itch5/add_order_message.hpp
        jb/itch5/add_order_mpid_message.cpp
        jb/itch5/add_order_mpid_message.hpp
        jb/itch5/array_based_order_book.cpp
        jb/itch5/array_based_order_book.hpp
        jb/itch5/base_decoders.cpp
        jb/itch5/base_decoders.hpp
        jb/itch5/base_encoders.hpp
        jb/itch5/broken_trade_message.cpp
        jb/itch5/broken_trade_message.hpp
        jb/itch5/char_list_field.hpp
        jb/itch5/char_list_validator.cpp
        jb/itch5/char_list_validator.hpp
        jb/itch5/check_offset.cpp
        jb/itch5/check_offset.hpp
        jb/itch5/compute_book.hpp
        jb/itch5/cross_trade_message.cpp
        jb/itch5/cross_trade_message.hpp
        jb/itch5/cross_type.hpp
        jb/itch5/decoder.hpp
        jb/itch5/generate_inside.hpp
        jb/itch5/ipo_quoting_period_update_message.cpp
        jb/itch5/ipo_quoting_period_update_message.hpp
        jb/itch5/make_socket_udp_common.hpp
        jb/itch5/make_socket_udp_recv.hpp
        jb/itch5/make_socket_udp_send.hpp
        jb/itch5/map_based_order_book.hpp
        jb/itch5/market_participant_position_message.cpp
        jb/itch5/market_participant_position_message.hpp
        jb/itch5/message_header.cpp
        jb/itch5/message_header.hpp
        jb/itch5/mold_udp_channel.cpp
        jb/itch5/mold_udp_channel.hpp
        jb/itch5/mold_udp_pacer.hpp
        jb/itch5/mold_udp_pacer_config.cpp
        jb/itch5/mold_udp_pacer_config.hpp
        jb/itch5/mold_udp_protocol_constants.hpp
        jb/itch5/mpid_field.hpp
        jb/itch5/mwcb_breach_message.cpp
        jb/itch5/mwcb_breach_message.hpp
        jb/itch5/mwcb_decline_level_message.cpp
        jb/itch5/mwcb_decline_level_message.hpp
        jb/itch5/net_order_imbalance_indicator_message.cpp
        jb/itch5/net_order_imbalance_indicator_message.hpp
        jb/itch5/order_book.hpp
        jb/itch5/order_cancel_message.cpp
        jb/itch5/order_cancel_message.hpp
        jb/itch5/order_delete_message.cpp
        jb/itch5/order_delete_message.hpp
        jb/itch5/order_executed_message.cpp
        jb/itch5/order_executed_message.hpp
        jb/itch5/order_executed_price_message.cpp
        jb/itch5/order_executed_price_message.hpp
        jb/itch5/order_replace_message.cpp
        jb/itch5/order_replace_message.hpp
        jb/itch5/price_field.hpp
        jb/itch5/price_levels.hpp
        jb/itch5/process_buffer_mlist.hpp
        jb/itch5/process_iostream.hpp
        jb/itch5/process_iostream_mlist.hpp
        jb/itch5/protocol_constants.hpp
        jb/itch5/quote_defaults.hpp
        jb/itch5/reg_sho_restriction_message.cpp
        jb/itch5/reg_sho_restriction_message.hpp
        jb/itch5/seconds_field.cpp
        jb/itch5/seconds_field.hpp
        jb/itch5/short_string_field.hpp
        jb/itch5/static_digits.hpp
        jb/itch5/stock_directory_message.cpp
        jb/itch5/stock_directory_message.hpp
        jb/itch5/stock_field.hpp
        jb/itch5/stock_trading_action_message.cpp
        jb/itch5/stock_trading_action_message.hpp
        jb/itch5/system_event_message.cpp
        jb/itch5/system_event_message.hpp
        jb/itch5/timestamp.cpp
        jb/itch5/timestamp.hpp
        jb/itch5/trade_message.cpp
        jb/itch5/trade_message.hpp
        jb/itch5/udp_config_common.cpp
        jb/itch5/udp_config_common.hpp
        jb/itch5/udp_receiver_config.cpp
        jb/itch5/udp_receiver_config.hpp
        jb/itch5/udp_sender_config.cpp
        jb/itch5/udp_sender_config.hpp
        jb/itch5/unknown_message.hpp
        )
target_link_libraries(jb_itch5 jb Boost::log Boost::program_options Boost::iostreams yaml-cpp)

add_library(jb_itch5_testing SHARED
        jb/itch5/testing/data.cpp
        jb/itch5/testing/data.hpp
        jb/itch5/testing/messages.cpp
        jb/itch5/testing/messages.hpp
        )
target_link_libraries(jb_itch5_testing jb_itch5 jb Boost::log Boost::program_options Boost::iostreams yaml-cpp)
set(jb_itch5_unit_tests
        jb/itch5/ut_add_order_message
        jb/itch5/ut_add_order_mpid_message
        jb/itch5/ut_array_based_order_book
        jb/itch5/ut_base_decoders
        jb/itch5/ut_base_encoders
        jb/itch5/ut_broken_trade_message
        jb/itch5/ut_char_list_field
        jb/itch5/ut_char_list_validator
        jb/itch5/ut_check_offset
        jb/itch5/ut_compute_book
        jb/itch5/ut_cross_trade_message
        jb/itch5/ut_cross_type
        jb/itch5/ut_generate_inside
        jb/itch5/ut_ipo_quoting_period_update_message
        jb/itch5/ut_make_socket_udp_common
        jb/itch5/ut_make_socket_udp_recv
        jb/itch5/ut_make_socket_udp_send
        jb/itch5/ut_map_based_order_book
        jb/itch5/ut_market_participant_position_message
        jb/itch5/ut_message_header
        jb/itch5/ut_mold_udp_pacer
        jb/itch5/ut_mold_udp_pacer_config
        jb/itch5/ut_mold_udp_channel
        jb/itch5/ut_mwcb_breach_message
        jb/itch5/ut_mwcb_decline_level_message
        jb/itch5/ut_net_order_imbalance_indicator_message
        jb/itch5/ut_order_book
        jb/itch5/ut_order_cancel_message
        jb/itch5/ut_order_delete_message
        jb/itch5/ut_order_executed_message
        jb/itch5/ut_order_executed_price_message
        jb/itch5/ut_order_replace_message
        jb/itch5/ut_price_field
        jb/itch5/ut_price_levels
        jb/itch5/ut_process_buffer_mlist
        jb/itch5/ut_process_iostream_mlist
        jb/itch5/ut_reg_sho_restriction_message
        jb/itch5/ut_seconds_field
        jb/itch5/ut_short_string_field
        jb/itch5/ut_static_digits
        jb/itch5/ut_stock_directory_message
        jb/itch5/ut_stock_trading_action_message
        jb/itch5/ut_system_event_message
        jb/itch5/ut_timestamp
        jb/itch5/ut_trade_message
        jb/itch5/ut_udp_receiver_config
        )

add_executable(jb_itch5_mold2inside jb/itch5/mold2inside.cpp)
target_link_libraries(jb_itch5_mold2inside jb_itch5 jb)
add_executable(jb_itch5_moldfeedhandler jb/itch5/moldfeedhandler.cpp)
target_link_libraries(jb_itch5_moldfeedhandler jb_itch5 jb_ehs jb)
add_executable(jb_itch5_moldreplay jb/itch5/moldreplay.cpp)
target_link_libraries(jb_itch5_moldreplay jb_itch5 jb_ehs jb)

add_executable(tools_itch5bookdepth tools/itch5bookdepth.cpp)
target_link_libraries(tools_itch5bookdepth jb_itch5 jb)
add_executable(tools_itch5eventdepth tools/itch5eventdepth.cpp)
target_link_libraries(tools_itch5eventdepth jb_itch5 jb)
add_executable(tools_itch5inside tools/itch5inside.cpp)
target_link_libraries(tools_itch5inside jb_itch5 jb)
add_executable(tools_itch5moldreplay tools/itch5moldreplay.cpp)
target_link_libraries(tools_itch5moldreplay jb_itch5 jb)
add_executable(tools_itch5stats tools/itch5stats.cpp)
target_link_libraries(tools_itch5stats jb_itch5 jb)
add_executable(tools_itch5trades tools/itch5trades.cpp)
target_link_libraries(tools_itch5trades jb_itch5 jb)
add_executable(tools_moldheartbeat tools/moldheartbeat.cpp)
target_link_libraries(tools_moldheartbeat jb_itch5 jb)

# TODO(coryan) - what to do with these
set(bin_SCRIPTS tools/benchmark_common.sh)

include(cmake/CompileProtos.cmake)
set(PROTOBUF_IMPORT_DIRS "${PROJECT_SOURCE_DIR}/ext/googleapis"
        "${PROJECT_SOURCE_DIR}/ext/gogoproto"
        "${PROJECT_SOURCE_DIR}/ext"
        )
PROTOBUF_GENERATE_CPP(PROTO_SOURCES PROTO_HDRS
        ${PROJECT_SOURCE_DIR}/ext/googleapis/google/api/http.proto
        ${PROJECT_SOURCE_DIR}/ext/googleapis/google/api/annotations.proto
        ${PROJECT_SOURCE_DIR}/ext/gogoproto/gogoproto/gogo.proto
        ${PROJECT_SOURCE_DIR}/ext/etcd/mvcc/mvccpb/kv.proto
        ${PROJECT_SOURCE_DIR}/ext/etcd/auth/authpb/auth.proto
        ${PROJECT_SOURCE_DIR}/ext/etcd/etcdserver/etcdserverpb/rpc.proto
        )
GRPC_GENERATE_CPP(GRPCPP_SOURCES GRPCPP_HDRS GRPCPP_MOCKS
        ${PROJECT_SOURCE_DIR}/ext/etcd/etcdserver/etcdserverpb/rpc.proto
        )
add_library(jb_etcd SHARED
        jb/etcd/active_completion_queue.cpp
        jb/etcd/active_completion_queue.hpp
        jb/etcd/completion_queue.hpp
        jb/etcd/completion_queue_base.cpp
        jb/etcd/completion_queue_base.hpp
        jb/etcd/detail/async_ops.hpp
        jb/etcd/detail/default_grpc_interceptor.hpp
        jb/etcd/detail/grpc_errors_annotations.hpp
        jb/etcd/detail/mocked_grpc_interceptor.hpp
        jb/etcd/detail/session_impl.hpp
        jb/etcd/grpc_errors.cpp
        jb/etcd/grpc_errors.hpp
        jb/etcd/leader_election_participant.cpp
        jb/etcd/leader_election_participant.hpp
        jb/etcd/leader_election_runner.cpp
        jb/etcd/leader_election_runner.hpp
        jb/etcd/log_promise_errors.cpp
        jb/etcd/log_promise_errors.hpp
        jb/etcd/prefix_end.cpp
        jb/etcd/prefix_end.hpp
        jb/etcd/session.cpp
        jb/etcd/session.hpp
        ${PROTO_SOURCES} ${PROTO_HDRS}
        ${GRPCPP_SOURCES} ${GRPCPP_HEADERS}
        )
target_link_libraries(jb_etcd ${GRPCPP_LIBRARIES} ${GRPC_LIBRARIES} ${PROTOBUF_LIBRARIES} jb)
set(jb_etcd_unit_tests
        jb/etcd/active_completion_queue_ut
        jb/etcd/completion_queue_ut
        jb/etcd/detail/leader_election_runner_impl_ut
        jb/etcd/detail/mocked_grpc_interceptor_ut
        jb/etcd/detail/session_impl_ut
        jb/etcd/grpc_errors_ut
        jb/etcd/log_promise_errors_ut
        jb/etcd/prefix_end_ut
        )

add_executable(jb_etcd_election_listener jb/etcd/election_listener.cpp)
target_link_libraries(jb_etcd_election_listener jb_etcd jb)

add_executable(jb_etcd_election_participant jb/etcd/election_participant.cpp)
target_link_libraries(jb_etcd_election_participant jb_etcd jb)

add_executable(jb_etcd_leader_election_participant_ut jb/etcd/leader_election_participant_ut.cpp)
target_link_libraries(jb_etcd_leader_election_participant_ut jb_etcd jb Boost::unit_test_framework)
target_compile_definitions(jb_etcd_leader_election_participant_ut
        PUBLIC -DBOOST_TEST_MAIN -DBOOST_TEST_DYN_LINK -DBOOST_TEST_MODULE=${target})

add_executable(jb_etcd_session_ut jb/etcd/session_ut.cpp)
target_link_libraries(jb_etcd_session_ut jb_etcd jb Boost::unit_test_framework)
target_compile_definitions(jb_etcd_session_ut
        PUBLIC -DBOOST_TEST_MAIN -DBOOST_TEST_DYN_LINK -DBOOST_TEST_MODULE=${target})

# ... determine if libraries are installed in the lib64 directory ...
get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
if ("${LIB64}" STREQUAL "TRUE")
    set(LIBSUFFIX 64)
else ()
    set(LIBSUFFIX "")
endif ()
set(INSTALL_LIB_DIR lib${LIBSUFFIX} CACHE PATH "Installation directory for libraries")

# ... generate a pkg-config for Jaybeams ...
set(PKG_CONFIG_REQUIRES "grpc++ grpc protobuf")
set(PKG_CONFIG_LIBDIR "\${prefix}/lib")
set(PKG_CONFIG_INCLUDEDIR "\${prefix}/include")
set(PKG_CONFIG_LIBS "-L\${libdir} -ljb")
set(PKG_CONFIG_CFLAGS "-I\${includedir}")
#CONFIGURE_FILE(
#        "${CMAKE_CURRENT_SOURCE_DIR}/templates/pkg-config.pc.in"
#        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc")

# ... define the install rules ...
install(TARGETS jb jb_testing jb_ehs jb_pitch2 jb_itch5 jb_etcd
        jb_etcd_election_participant jb_etcd_election_listener tools_itch5bookdepth tools_itch5eventdepth
        tools_itch5inside tools_itch5moldreplay tools_itch5stats tools_itch5trades tools_moldheartbeat
        jb_itch5_mold2inside jb_itch5_moldfeedhandler jb_itch5_moldreplay
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION ${INSTALL_LIB_DIR})
install(PROGRAMS jb/itch5/bm_order_book_report.Rmd jb/itch5/bm_order_book_analyze.R jb/itch5/bm_order_book_analyze.R
        tools/benchmark_common.sh tools/benchmark_teardown.sh
        DESTINATION bin)
install(DIRECTORY jb/ DESTINATION include/jb FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY ${CMAKE_BINARY_DIR}/etcd/ DESTINATION include/etcd FILES_MATCHING PATTERN "*.h")
install(DIRECTORY ${CMAKE_BINARY_DIR}/gogoproto/ DESTINATION include/gogoproto FILES_MATCHING PATTERN "*.h")
install(DIRECTORY ${CMAKE_BINARY_DIR}/google/ DESTINATION include/google FILES_MATCHING PATTERN "*.h")
#install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc" DESTINATION lib/pkgconfig)

include(GNUInstallDirs)
add_definitions(-DJB_SYSCONFDIR="${CMAKE_INSTALL_FULL_SYSCONFDIR}" -DJB_BINDIR="${CMAKE_INSTALL_FULL_BINDIR}")

# A helper function to wrap the common configuration for tests ...
function(jb_create_unit_test_targets)
    foreach (fname ${ARGN})
        string(REPLACE "/" "_" target ${fname})
        string(REGEX MATCH ".*_ut$" is_ut ${target})
        if (NOT is_ut)
            string(REGEX MATCH ".*_ut_.*" is_ut ${target})
        endif ()
        if (NOT is_ut)
            string(REGEX MATCH "^ut_.*" is_ut ${target})
        endif ()
        if (is_ut)
            add_executable(${target} ${fname}.cpp)
            get_target_property(tname ${target} NAME)
            target_compile_definitions(${target} PUBLIC
                    -DBOOST_TEST_MAIN -DBOOST_TEST_DYN_LINK -DBOOST_TEST_MODULE=${target})
            target_link_libraries(${target} jb Boost::unit_test_framework)
        endif ()
    endforeach ()
endfunction(jb_create_unit_test_targets)

set(all_unit_tests
        ${jb_unit_tests} ${jb_testing_unit_tests}
        ${jb_pitch2_unit_tests}
        ${jb_ehs_unit_tests}
        ${jb_itch5_unit_tests}
        ${jb_etcd_unit_tests}
        )
jb_create_unit_test_targets(${all_unit_tests})

if (NOT JB_DISABLE_TESTS)
    # ... turn on testing support in CMake ...
    enable_testing()

    foreach (fname ${all_unit_tests})
        string(REPLACE "/" "_" target ${fname})
        add_test(${target} ${target})
    endforeach ()

    # ... add the right libraries to all the unit tests, and also turn them into tests ...
    foreach (fname ${jb_testing_unit_tests})
        string(REPLACE "/" "_" target ${fname})
        target_link_libraries(${target} jb_testing)
    endforeach ()
    foreach (fname ${jb_pitch2_unit_tests})
        string(REPLACE "/" "_" target ${fname})
        target_link_libraries(${target} jb_pitch2)
    endforeach ()
    foreach (fname ${jb_itch5_unit_tests})
        string(REPLACE "/" "_" target ${fname})
        target_link_libraries(${target} jb_itch5_testing jb_itch5)
    endforeach ()
    foreach (fname ${jb_ehs_unit_tests})
        string(REPLACE "/" "_" target ${fname})
        target_link_libraries(${target} jb_ehs)
    endforeach ()
    foreach (fname ${jb_etcd_unit_tests})
        string(REPLACE "/" "_" target ${fname})
        target_link_libraries(${target} jb_etcd)
    endforeach ()
    foreach (target
            jb_etcd_detail_leader_election_runner_impl_ut
            jb_etcd_detail_mocked_grpc_interceptor_ut
            jb_etcd_detail_session_impl_ut
            jb_itch5_ut_compute_book
            jb_itch5_ut_make_socket_udp_common
            jb_itch5_ut_make_socket_udp_recv
            jb_itch5_ut_make_socket_udp_send
            jb_itch5_ut_mold_udp_channel
            jb_itch5_ut_mold_udp_pacer
            jb_itch5_ut_process_buffer_mlist
            jb_itch5_ut_process_iostream_mlist
            jb_ut_config_files_location
            )
        target_include_directories(${target} PRIVATE ${PROJECT_SOURCE_DIR}/ext/googletest/googlemock)
        target_link_libraries(${target} jb_gmock)
    endforeach ()
endif (NOT JB_DISABLE_TESTS)

include(cmake/FindFFTW.cmake)
if (FFTW3_FOUND)
    add_subdirectory(jb/fftw)
endif (FFTW3_FOUND)

include(FindOpenCL)
if (OpenCL_FOUND)
    add_subdirectory(jb/opencl)
endif (OpenCL_FOUND)

include(cmake/FindCLFFT.cmake)
if (CLFFT_FOUND)
    add_subdirectory(jb/clfft)
    add_subdirectory(jb/tde)
endif (CLFFT_FOUND)

file(GLOB_RECURSE ALL_CXX_SOURCE_FILES jb/*.[hc]pp examples/*.[hc]pp tools/*.[hc]pp)
find_program(CLANG_FORMAT clang-format)
if (CLANG_FORMAT)
    add_custom_target(clang-format
            COMMAND ${CLANG_FORMAT} -i ${ALL_CXX_SOURCE_FILES})
    add_custom_target(check-style
            COMMAND git diff --exit-code
            DEPENDS clang-format)
else()
    add_custom_target(check-style)
endif (CLANG_FORMAT)
