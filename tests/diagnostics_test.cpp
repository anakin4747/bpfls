#include <memory>
#include <string>
#include <vector>

// cppspec defines macros such as `it`, which clash with identifiers in
// bpftrace's headers, so project headers must be included first.
#include "diagnostics.h"

#include "cppspec.hpp"

namespace {

// The messages of all diagnostics published for `text`.
std::vector<std::string> messages_for(const std::string &text)
{
    bpfls::Document document("file:///t.bt", text);
    std::vector<std::string> result;
    for (const auto &d : bpfls::diagnostics(document)) {
        const auto *m = std::get_if<std::string>(&d.message);
        result.push_back(m != nullptr ? *m : "<not plain text>");
    }
    return result;
}

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

describe diagnostics_list_test("diagnostics", $ {
    it("includes bpftrace's warning for while loops", _ {
        auto messages = messages_for("begin { while (1) { } }");
        expect(messages.size()).to_equal(1UL);
        if (messages.size() == 1UL)
            expect(messages.front().starts_with("While loops are deprecated"))
                .to_be_true();
    });

    it("includes bpftrace's error for sargN", _ {
        auto messages = messages_for("uprobe:/b:f { $x = sarg0; }");
        expect(messages.size()).to_equal(1UL);
        if (messages.size() == 1UL)
            expect(messages.front().starts_with("sarg* is deleted"))
                .to_be_true();
    });

    it("includes bpfls's own lints", _ {
        auto messages = messages_for("kprobe:f { @[stack] = count(); }");
        expect(messages.size()).to_equal(1UL);
        if (messages.size() == 1UL)
            expect(messages.front().starts_with("stack was removed"))
                .to_be_true();
    });
});

CPPSPEC_MAIN(diagnostics_test, diagnostics_list_test);
