cmake_minimum_required(VERSION 3.24)

project(bpfls LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Generate compile_commands.json and link it into the source root so clangd
# and other tools find it regardless of the build directory's name.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
file(CREATE_LINK
  ${CMAKE_BINARY_DIR}/compile_commands.json
  ${CMAKE_SOURCE_DIR}/compile_commands.json
  SYMBOLIC
)

# Only the parts of bpftrace needed to parse a script are compiled; the rest
# of bpftrace (LLVM codegen, runtime, ...) is never built.
#
# bpfls carries a few changes to bpftrace as patches (patches/bpftrace/).
# The submodule is never modified: its whole src/ is copied into the build
# tree, patched there, and only that copy is compiled and put on the include
# path. Copying all of it, not just the patched files, matters: bpftrace's
# quoted includes are looked up next to the including file first, so a
# partial copy lets one translation unit see both the original and the
# patched version of a header. This re-runs whenever a patch or any bpftrace
# source file changes.
find_package(Git REQUIRED)
file(GLOB BPFTRACE_PATCHES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/patches/bpftrace/*.patch)
list(SORT BPFTRACE_PATCHES)
file(GLOB_RECURSE BPFTRACE_ORIGINALS CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/bpftrace/src/*)

set(BPFTRACE_PATCHED_DIR ${CMAKE_CURRENT_BINARY_DIR}/bpftrace-patched)
set(BPFTRACE_SRC ${BPFTRACE_PATCHED_DIR}/src)

# The bpftrace files compiled into bpfls.
set(BPFTRACE_SOURCES
  parser.cpp
  btf/btf.cpp
  log.cpp
  probe_types.cpp
  types.cpp
  ast/ast.cpp
  ast/context.cpp
  ast/diagnostic.cpp
  ast/location.cpp
  ast/pass_manager.cpp
  util/int_parser.cpp
  util/result.cpp
  util/strings.cpp
)
list(TRANSFORM BPFTRACE_SOURCES PREPEND ${BPFTRACE_SRC}/)

# The patched copy lives inside the bpfls repo; GIT_CEILING_DIRECTORIES stops
# `git apply` from finding that repo, so it patches paths relative to the
# working directory instead of the repo root.
set(apply_patches "")
foreach(patch IN LISTS BPFTRACE_PATCHES)
  list(APPEND apply_patches
    COMMAND ${CMAKE_COMMAND} -E env
            GIT_CEILING_DIRECTORIES=${CMAKE_CURRENT_BINARY_DIR}
            ${GIT_EXECUTABLE} -C ${BPFTRACE_PATCHED_DIR} apply ${patch})
endforeach()

set(BPFTRACE_STAMP ${BPFTRACE_PATCHED_DIR}.stamp)
add_custom_command(
  OUTPUT ${BPFTRACE_STAMP} ${BPFTRACE_SOURCES}
  COMMAND ${CMAKE_COMMAND} -E rm -rf ${BPFTRACE_PATCHED_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/bpftrace/src ${BPFTRACE_SRC}
  ${apply_patches}
  COMMAND ${CMAKE_COMMAND} -E touch ${BPFTRACE_STAMP}
  DEPENDS ${BPFTRACE_PATCHES} ${BPFTRACE_ORIGINALS}
  COMMENT "Copying bpftrace and applying bpfls patches"
  VERBATIM
)
add_custom_target(bpftrace_patched DEPENDS ${BPFTRACE_STAMP})

add_library(bpftrace_parser STATIC
  src/bpftrace_shims.cpp
  ${BPFTRACE_SOURCES}
)
# Headers are only ever taken from the patched copy, which must exist before
# anything that includes them is compiled.
add_dependencies(bpftrace_parser bpftrace_patched)
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

# lsp-framework provides the LSP types, JSON-RPC and the stdio transport.
# Its types are generated from the LSP meta model at build time.
set(LSP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LSP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LSP_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(lsp-framework EXCLUDE_FROM_ALL)

add_library(bpfls_core STATIC
  src/diagnostics.cpp
  src/document.cpp
  src/hover_docs.cpp
  src/server.cpp
)
target_include_directories(bpfls_core PUBLIC src)
target_link_libraries(bpfls_core PUBLIC bpftrace_parser lsp)

# Nearly all of every translation unit is the C++ standard library (mostly
# <ranges>, <format> and <chrono>). Precompile it once per target instead of
# parsing it again for every file. Only standard headers are listed, so
# nothing generated has to exist first and nothing needs keeping in sync.
set(BPFLS_PCH
  <algorithm> <chrono> <format> <functional> <map> <memory>
  <optional> <ranges> <string> <unordered_map> <variant> <vector>
)
foreach(target IN ITEMS bpftrace_parser bpfls_core lsp)
  target_precompile_headers(${target} PRIVATE ${BPFLS_PCH})
endforeach()

add_executable(bpfls src/main.cpp)
target_link_libraries(bpfls PRIVATE bpfls_core)
# The docs bpfls shows are compiled in, so the binary is all there is to
# install.
include(GNUInstallDirs)
install(TARGETS bpfls RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# The tests pull in cppspec and link LLVM into every test executable, which
# makes the build much slower and hungrier for memory. They are off unless
# asked for; `make` and `nix flake check` turn them on.
option(BPFLS_BUILD_TESTS "Build the unit and end-to-end tests" OFF)
if(BPFLS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
