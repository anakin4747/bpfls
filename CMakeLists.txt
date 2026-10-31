cmake_minimum_required(VERSION 3.24)

project(bpfls LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
  cppspec
  GIT_REPOSITORY https://github.com/toroidal-code/cppspec
  GIT_TAG        v1.0.2
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(cppspec)

# bpftrace uses CMAKE_SOURCE_DIR/CMAKE_BINARY_DIR internally, so it cannot be
# pulled in with FetchContent/add_subdirectory; build it as its own project.
include(ExternalProject)

ExternalProject_Add(bpftrace
  GIT_REPOSITORY https://github.com/bpftrace/bpftrace
  GIT_TAG        f0c754756b8c4a2b05468b6298db68db2746fe0a
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DBUILD_TESTING=OFF
    -DENABLE_MAN=OFF
  INSTALL_COMMAND ""
)

enable_testing()
add_subdirectory(tests)
