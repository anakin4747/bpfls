#include <optional>
#include <string>

// cppspec defines macros such as `it`, which clash with identifiers in
// bpftrace's headers, so project headers must be included first.
#include "document.h"
#include "formatting.h"

#include "cppspec.hpp"

namespace {

// Formats `text` as a fresh document.
std::optional<std::string> format_text(const std::string &text)
{
    bpfls::Document doc("file:///t.bt", text);
    return bpfls::format(doc);
}

} // namespace

describe formatting_test("format", $ {
    it("lays out a script the way bpftrace --fmt does", _ {
        expect(format_text("begin{print(1)}").value_or("<nullopt>"))
            .to_equal(std::string("begin\n{\n  print(1);\n}\n"));
    });

    it("keeps comments", _ {
        auto out = format_text("// greet\nbegin { print(1); /* one */ }\n");
        expect(out.has_value()).to_be_true();
        if (out.has_value()) {
            expect(out->find("// greet") != std::string::npos).to_be_true();
            expect(out->find("one") != std::string::npos).to_be_true();
        }
    });

    it("leaves formatted text unchanged", _ {
        auto once = format_text("begin{$x=1;if($x){print($x)}}\nend{exit()}");
        expect(once.has_value()).to_be_true();
        if (once.has_value())
            expect(format_text(*once) == once).to_be_true();
    });

    it("refuses to format a script with syntax errors", _ {
        expect(format_text("begin { $x = ; }\nend { print(1); }") ==
               std::nullopt)
            .to_be_true();
    });
});

CPPSPEC_MAIN(formatting_test);
