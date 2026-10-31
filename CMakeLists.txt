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

enable_testing()
add_subdirectory(tests)
