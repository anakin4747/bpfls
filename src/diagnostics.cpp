#include "diagnostics.h"

#include <string>

namespace bpfls {

namespace {

// bpftrace positions are 1-based; LSP positions are 0-based.
// TODO: bpftrace columns count bytes while LSP counts UTF-16 code units by
// default, so columns are only right for ASCII text.
lsp::Position to_lsp(const bpftrace::ast::SourceLocation::Position &pos)
{
    return {
        .line = static_cast<lsp::Uint>(pos.line - 1),
        .character = static_cast<lsp::Uint>(pos.column - 1),
    };
}

} // namespace

lsp::Diagnostic to_lsp(const bpftrace::ast::Diagnostic &diagnostic,
                       lsp::DiagnosticSeverity severity)
{
    std::string message = diagnostic.msg();
    for (const auto &hint : diagnostic.hints())
        message += "\nHINT: " + hint;

    const auto &loc = diagnostic.loc()->current;
    return {
        .range = { .start = to_lsp(loc.begin), .end = to_lsp(loc.end) },
        .message = std::move(message),
        .severity = severity,
        .source = "bpftrace",
    };
}

std::vector<lsp::Diagnostic> diagnostics(Document &document)
{
    using Severity = bpftrace::ast::Diagnostic::Severity;

    std::vector<lsp::Diagnostic> result;
    const auto &all = document.ast().diagnostics();
    all.foreach(Severity::Error, [&](const bpftrace::ast::Diagnostic &d) {
        result.push_back(to_lsp(d, lsp::DiagnosticSeverity::Error));
    });
    all.foreach(Severity::Warning, [&](const bpftrace::ast::Diagnostic &d) {
        result.push_back(to_lsp(d, lsp::DiagnosticSeverity::Warning));
    });
    return result;
}

} // namespace bpfls
