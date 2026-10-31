cmake_minimum_required(VERSION 3.24)

project(bpfls LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# bpftrace uses CMAKE_SOURCE_DIR/CMAKE_BINARY_DIR internally, so it cannot be
# pulled in with add_subdirectory; build the submodule as its own project.
include(ExternalProject)

ExternalProject_Add(bpftrace
  SOURCE_DIR       ${CMAKE_CURRENT_SOURCE_DIR}/bpftrace
  DOWNLOAD_COMMAND ""
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DBUILD_TESTING=OFF
    -DENABLE_MAN=OFF
  INSTALL_COMMAND ""
)

enable_testing()
add_subdirectory(tests)
