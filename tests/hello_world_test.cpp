#include <string>

#include "cppspec.hpp"

describe hello_world_test("Hello world", $ {
  it("greets the world", _ {
    std::string greeting = "Hello, world!";
    expect(greeting).to_equal("Hello, world!");
  });
});

CPPSPEC_MAIN(hello_world_test);
