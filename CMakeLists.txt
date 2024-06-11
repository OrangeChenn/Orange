cmake_minimum_required(VERSION 3.6)
project(Orange)

include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

set(LIB_SRC
    src/log.cc
    src/util.cc
    src/config.cc
    src/daemon.cc
    src/thread.cc
    src/mutex.cc
    src/fiber.cc
    src/scheduler.cc
    src/iomanager.cc
    src/timer.cc
    src/hook.cc
    src/fd_manager.cc
    src/address.cc
    src/socket.cc
    src/bytearray.cc
    src/tcp_server.cc
    src/stream.cc
    src/uri.rl.cc
    src/http/http.cc
    src/http/http_parser.cc
    src/http/http11_parser.rl.cc
    src/http/httpclient_parser.rl.cc
    src/http/http_session.cc
    src/http/http_server.cc
    src/http/http_connection.cc
    src/http/servlet.cc
    src/stream/socket_stream.cc
    )

include_directories(. src)
add_library(orange SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(orange)

find_package(yaml-cpp REQUIRED)
include_directories(${YAML_CPP_INCLUDE_DIR})

set(LIBS
    orange
    dl
    yaml-cpp
    pthread
    )

orange_add_executable(test "tests/test.cc" orange "${LIBS}")
orange_add_executable(test_config "tests/test_config.cc" orange "${LIBS}")
orange_add_executable(test_thread "tests/test_thread.cc" orange "${LIBS}")
orange_add_executable(test_util "tests/test_util.cc" orange "${LIBS}")
orange_add_executable(test_fiber "tests/test_fiber.cc" orange "${LIBS}")
orange_add_executable(test_scheduler "tests/test_scheduler.cc" orange "${LIBS}")
orange_add_executable(test_iomanager "tests/test_iomanager.cc" orange "${LIBS}")
orange_add_executable(test_hook "tests/test_hook.cc" orange "${LIBS}")
orange_add_executable(test_address "tests/test_address.cc" orange "${LIBS}")
orange_add_executable(test_socket "tests/test_socket.cc" orange "${LIBS}")
orange_add_executable(test_bytearray "tests/test_bytearray.cc" orange "${LIBS}")
orange_add_executable(test_http "tests/test_http.cc" orange "${LIBS}")
orange_add_executable(test_http_parser "tests/test_http_parser.cc" orange "${LIBS}")
orange_add_executable(test_tcp_server "tests/test_tcp_server.cc" orange "${LIBS}")
orange_add_executable(echo_server "examples/echo_server.cc" orange "${LIBS}")
orange_add_executable(test_http_server "tests/test_http_server.cc" orange "${LIBS}")
orange_add_executable(test_http_connection "tests/test_http_connection.cc" orange "${LIBS}")
orange_add_executable(test_uri "tests/test_uri.cc" orange "${LIBS}")
orange_add_executable(my_http_server "samples/my_http_server.cc" orange "${LIBS}")
orange_add_executable(test_daemon "tests/test_daemon.cc" orange "${LIBS}")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
