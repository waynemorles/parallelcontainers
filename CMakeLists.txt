cmake_minimum_required(VERSION 3.0)
project(cocurrent C CXX)

set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_STANDARD 17)

set(CXX_FLAGS
    -std=c++11
    -g
    -Wall
    -Wextra
    -Wformat
    -Wsign-compare
    -Wno-unused-parameter
    -Wno-unused-variable
    -Woverloaded-virtual
    -Wno-missing-field-initializers
    -fno-omit-frame-pointer
    -D__STDC_FORMAT_MACROS
)

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_C_STANDARD 99)

set(C_FLAGS
    -g
    -Wall
    -Wextra
    -Wformat
    -Wsign-compare
    -Wno-unused-parameter
    -Wno-unused-variable
    -Wno-missing-field-initializers
    -fno-omit-frame-pointer
)

string(REPLACE ";" " " CXX_FLAGS_TEMP "${CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_FLAGS_TEMP}")

string(REPLACE ";" " " C_FLAGS_TEMP "${C_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_FLAGS_TEMP}")

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -DNDEBUG")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -ggdb")
endif()

MESSAGE(STATUS CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})

if (APPLE)
    set(CMAKE_MACOSX_RPATH 1)
endif()

include(cmake/common.cmake)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

include_directories(${PROJECT_SOURCE_DIR}/include)

# use gcov
OPTION (ENABLE_COVERAGE "Use gcov" OFF)
MESSAGE(STATUS ENABLE_COVERAGE=${ENABLE_COVERAGE})
if(ENABLE_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

# gcc address sanitize and jemalloc
OPTION (ENABLE_SANITIZE "Use gcc address sanitize" OFF)
MESSAGE(STATUS ENABLE_SANITIZE=${ENABLE_SANITIZE})
if(ENABLE_SANITIZE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif()

message(STATUS "CMAKE_CXX_FLAGS: " ${CMAKE_CXX_FLAGS})
message(STATUS "CMAKE_C_FLAGS: " ${CMAKE_C_FLAGS})

OPTION (BUILD_TESTS "build tests" ON)
MESSAGE(STATUS BUILD_TESTS=${BUILD_TESTS})

if (BUILD_TESTS)
    add_subdirectory(tests)
endif()

