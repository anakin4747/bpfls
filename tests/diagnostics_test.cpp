#include <memory>
#include <string>

// cppspec defines macros such as `it`, which clash with identifiers in
// bpftrace's headers, so project headers must be included first.
#include "diagnostics.h"

#include "cppspec.hpp"

namespace {

// A bpftrace diagnostic spanning 1-based line 2, columns 5 up to 9.
bpftrace::ast::Diagnostic make_diagnostic()
{
    bpftrace::ast::SourceLocation loc;
    loc.begin = { .line = 2, .column = 5 };
    loc.end = { .line = 2, .column = 9 };
    bpftrace::ast::Diagnostic diagnostic(
        std::make_shared<bpftrace::ast::LocationChain>(loc));
    diagnostic << "syntax: expected expression";
    return diagnostic;
}

// The diagnostic's message if it is plain text.
std::string message_of(const lsp::Diagnostic &d)
{
    const auto *text = std::get_if<std::string>(&d.message);
    return text != nullptr ? *text : "<not plain text>";
}

} // namespace

describe diagnostics_test("to_lsp", $ {
    it("keeps bpftrace's message as is", _ {
        auto d = bpfls::to_lsp(make_diagnostic(),
                               lsp::DiagnosticSeverity::Error);
        expect(message_of(d))
            .to_equal(std::string("syntax: expected expression"));
    });

    it("converts the range to 0-based positions", _ {
        auto d = bpfls::to_lsp(make_diagnostic(),
                               lsp::DiagnosticSeverity::Error);
        expect(d.range.start.line).to_equal(1U);
        expect(d.range.start.character).to_equal(4U);
        expect(d.range.end.line).to_equal(1U);
        expect(d.range.end.character).to_equal(8U);
    });

    it("sets the severity and source", _ {
        auto d = bpfls::to_lsp(make_diagnostic(),
                               lsp::DiagnosticSeverity::Warning);
        expect(d.severity.has_value()).to_be_true();
        if (d.severity.has_value()) {
            expect(*d.severity == lsp::DiagnosticSeverity::Warning)
                .to_be_true();
        }
        expect(d.source.value_or("")).to_equal(std::string("bpftrace"));
    });

    it("appends hints the way bpftrace prints them", _ {
        auto diagnostic = make_diagnostic();
        diagnostic.addHint() << "first hint";
        diagnostic.addHint() << "second hint";
        auto d = bpfls::to_lsp(diagnostic, lsp::DiagnosticSeverity::Error);
        expect(message_of(d))
            .to_equal(std::string("syntax: expected expression\n"
                                  "HINT: first hint\n"
                                  "HINT: second hint"));
    });
});

CPPSPEC_MAIN(diagnostics_test);
