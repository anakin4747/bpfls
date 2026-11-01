add_subdirectory(cppspec)

file(GLOB_RECURSE tests CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp)

foreach(test IN LISTS tests)
  add_spec(${test} "")
  cmake_path(GET test STEM test_name)
  target_link_libraries(${test_name} bpfls_core)
  target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()
