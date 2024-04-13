cmake_minimum_required(VERSION 3.6)
project(Orange)

include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

set(LIB_SRC
    src/log.cc
    src/util.cc
    src/config.cc
    src/thread.cc
    src/mutex.cc
    )

include_directories(src)
add_library(orange SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(orange)

find_package(yaml-cpp REQUIRED)
include_directories(${YAML_CPP_INCLUDE_DIR})

set(LIBS
    orange
    yaml-cpp
    pthread
    )

add_executable(test tests/test.cc)
add_dependencies(test orange)
force_redefine_file_macro_for_sources(test)
target_link_libraries(test ${LIBS})

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config orange)
force_redefine_file_macro_for_sources(test_config)
target_link_libraries(test_config ${LIBS})

add_executable(test_thread tests/test_thread.cc)
add_dependencies(test_thread orange)
force_redefine_file_macro_for_sources(test_thread)
target_link_libraries(test_thread ${LIBS})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
