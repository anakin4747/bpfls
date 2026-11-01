cmake_minimum_required(VERSION 3.24)

project(bpfls LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Only the parts of bpftrace needed to parse a script are compiled; the rest
# of bpftrace (LLVM codegen, runtime, ...) is never built.
set(BPFTRACE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/bpftrace/src)

add_library(bpftrace_parser STATIC
  src/bpftrace_shims.cpp
  ${BPFTRACE_SRC}/btf/btf.cpp
  ${BPFTRACE_SRC}/log.cpp
  ${BPFTRACE_SRC}/parser.cpp
  ${BPFTRACE_SRC}/probe_types.cpp
  ${BPFTRACE_SRC}/types.cpp
  ${BPFTRACE_SRC}/ast/ast.cpp
  ${BPFTRACE_SRC}/ast/context.cpp
  ${BPFTRACE_SRC}/ast/diagnostic.cpp
  ${BPFTRACE_SRC}/ast/location.cpp
  ${BPFTRACE_SRC}/ast/pass_manager.cpp
  ${BPFTRACE_SRC}/util/int_parser.cpp
  ${BPFTRACE_SRC}/util/result.cpp
  ${BPFTRACE_SRC}/util/strings.cpp
)
target_include_directories(bpftrace_parser PUBLIC ${BPFTRACE_SRC})
# Some of these files also contain runtime code we never call. Put every
# function in its own section so the linker can drop the unused ones (and
# their references into the rest of bpftrace).
target_compile_options(bpftrace_parser PUBLIC -ffunction-sections -fdata-sections)
target_link_options(bpftrace_parser PUBLIC -Wl,--gc-sections)

find_package(LLVM REQUIRED CONFIG)
target_include_directories(bpftrace_parser SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS})
target_compile_definitions(bpftrace_parser PUBLIC ${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(LLVM_LIBS support)
target_link_libraries(bpftrace_parser PUBLIC ${LLVM_LIBS})

add_library(bpfls_core STATIC src/document.cpp)
target_include_directories(bpfls_core PUBLIC src)
target_link_libraries(bpfls_core PUBLIC bpftrace_parser)

enable_testing()
add_subdirectory(tests)
