# End-to-end tests run the real bpfls binary as a child process. They are
# labelled e2e, which `make tests` skips and `make e2e` selects; CI runs them
# with every other test through `nix flake check`.

file(GLOB tests CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp)

foreach(test IN LISTS tests)
  add_spec(${test} "")
  cmake_path(GET test STEM test_name)
  target_link_libraries(${test_name} lsp)
  target_compile_definitions(${test_name}
    PRIVATE BPFLS_EXECUTABLE="$<TARGET_FILE:bpfls>")
  add_dependencies(${test_name} bpfls)
  # A server that never exits must fail the test, not hang CI.
  set_tests_properties(${test_name} PROPERTIES LABELS e2e TIMEOUT 30)
endforeach()
