// cppspec defines macros such as `it`, which may clash with identifiers in
// other headers, so it must be included last.
#include <lsp/json/parser.h>
#include <lsp/types.h>

#include "cppspec.hpp"

describe lsp_test("lsp-framework", $ {
    it("deserializes LSP JSON into the generated types", _ {
        auto json = lsp::json::Parser(R"({"line": 3, "character": 7})").parse();
        lsp::Position pos{};
        lsp::fromJson(std::move(json), pos);
        expect(pos.line).to_equal(3U);
        expect(pos.character).to_equal(7U);
    });
});

CPPSPEC_MAIN(lsp_test);
