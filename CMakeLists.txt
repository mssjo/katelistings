
cmake_minimum_required(VERSION 3.10)


project(katelistings)
set(CMAKE_CXX_COMPILER "/usr/bin/g++-7")

set(CMAKE_CXX_STANDARD 17)
add_compile_options(-fmax-errors=10)


file(GLOB SOURCES src/*.cpp)
file(GLOB UTIL_SOURCES lib/util/file_parser.cpp lib/util/src/dom_element.cpp lib/util/src/file_utils.cpp lib/util/char_utils.cpp)

add_definitions(-DKATELISTINGS_DIR="${CMAKE_SOURCE_DIR}")

add_executable (katelistings ${SOURCES} ${UTIL_SOURCES})
add_executable (map_languages map_languages.cpp ${UTIL_SOURCES})

include_directories(include/)
include_directories(lib/util/)

