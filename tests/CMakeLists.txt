add_subdirectory(cppspec)

file(GLOB_RECURSE tests CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp)

foreach(test IN LISTS tests)
  add_spec(${test} "")
endforeach()
