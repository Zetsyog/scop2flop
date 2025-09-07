cmake_minimum_required(VERSION 3.22)
project(scop2flop)

set(SCOP2FLOP_VERSION_MAJOR 0)
set(SCOP2FLOP_VERSION_MINOR 1)
set(SCOP2FLOP_VERSION_PATCH 0)
set(SCOP2FLOP_VERSION
    "${SCOP2FLOP_VERSION_MAJOR}.${SCOP2FLOP_VERSION_MINOR}.${SCOP2FLOP_VERSION_PATCH}"
)

set(CMAKE_C_STANDARD 11)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

find_package(PkgConfig REQUIRED)
find_package(Git REQUIRED)
pkg_check_modules(GMP REQUIRED gmp)

include(cmake/osl.cmake)
include(cmake/clan.cmake)

pkg_check_modules(BARVINOK REQUIRED IMPORTED_TARGET GLOBAL barvinok)

add_executable(scop2flop src/main.c)
add_dependencies(scop2flop osl clan)

target_include_directories(scop2flop PUBLIC ${BARVINOK_INCLUDE_DIRS})
message(STATUS "barvinok libs: ${BARVINOK_LIBRARIES}")
target_link_libraries(scop2flop PRIVATE osl clan ${BARVINOK_LIBRARIES} stdc++ m)
