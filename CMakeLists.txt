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
set(BPFTRACE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/bpftrace/src)

# bpfls carries a few changes to bpftrace as patches (patches/bpftrace/).
# The submodule is never modified: the files the patches touch are copied
# into the build tree, patched there, and compiled instead of the originals.
# This re-runs whenever a patch or an original file changes.
find_package(Git REQUIRED)
file(GLOB BPFTRACE_PATCHES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/patches/bpftrace/*.patch)
list(SORT BPFTRACE_PATCHES)

# Every file touched by a patch must be listed here; `git apply` fails loudly
# if one is missing. Files that include a patched header by a path relative
# to themselves (e.g. `#include "diagnostic.h"` in src/ast/) must be listed
# too, so that include finds the patched copy rather than the original;
# otherwise one translation unit could see both versions of the header.
set(BPFTRACE_PATCHED_FILES
  src/parser.cpp
  src/parser.h
  src/ast/ast.h
  src/ast/diagnostic.cpp
  src/ast/diagnostic.h
)
set(BPFTRACE_PATCHED_DIR ${CMAKE_CURRENT_BINARY_DIR}/bpftrace-patched)

set(patched_outputs "")
set(patched_originals "")
set(copy_originals "")
foreach(file IN LISTS BPFTRACE_PATCHED_FILES)
  list(APPEND patched_outputs ${BPFTRACE_PATCHED_DIR}/${file})
  list(APPEND patched_originals ${CMAKE_CURRENT_SOURCE_DIR}/bpftrace/${file})
  cmake_path(GET file PARENT_PATH dir)
  list(APPEND copy_originals
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BPFTRACE_PATCHED_DIR}/${dir}
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/bpftrace/${file}
            ${BPFTRACE_PATCHED_DIR}/${file})
endforeach()

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

add_custom_command(
  OUTPUT ${patched_outputs}
  COMMAND ${CMAKE_COMMAND} -E rm -rf ${BPFTRACE_PATCHED_DIR}
  ${copy_originals}
  ${apply_patches}
  DEPENDS ${BPFTRACE_PATCHES} ${patched_originals}
  COMMENT "Applying bpfls patches to bpftrace"
  VERBATIM
)

add_library(bpftrace_parser STATIC
  src/bpftrace_shims.cpp
  ${BPFTRACE_PATCHED_DIR}/src/parser.cpp
  ${BPFTRACE_PATCHED_DIR}/src/parser.h
  ${BPFTRACE_PATCHED_DIR}/src/ast/ast.h
  ${BPFTRACE_PATCHED_DIR}/src/ast/diagnostic.cpp
  ${BPFTRACE_PATCHED_DIR}/src/ast/diagnostic.h
  ${BPFTRACE_SRC}/btf/btf.cpp
  ${BPFTRACE_SRC}/log.cpp
  ${BPFTRACE_SRC}/probe_types.cpp
  ${BPFTRACE_SRC}/types.cpp
  ${BPFTRACE_SRC}/ast/ast.cpp
  ${BPFTRACE_SRC}/ast/context.cpp
  ${BPFTRACE_SRC}/ast/location.cpp
  ${BPFTRACE_SRC}/ast/pass_manager.cpp
  ${BPFTRACE_SRC}/util/int_parser.cpp
  ${BPFTRACE_SRC}/util/result.cpp
  ${BPFTRACE_SRC}/util/strings.cpp
)
# Patched headers must shadow the originals for every consumer, otherwise
# e.g. Parser would have two different layouts in one program.
target_include_directories(bpftrace_parser BEFORE PUBLIC
  ${BPFTRACE_PATCHED_DIR}/src)
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

add_executable(bpfls src/main.cpp)
target_link_libraries(bpfls PRIVATE bpfls_core)
# The docs bpfls shows are compiled in, so the binary is all there is to
# install.
include(GNUInstallDirs)
install(TARGETS bpfls RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

enable_testing()
add_subdirectory(tests)
