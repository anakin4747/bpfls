#include "diagnostics.h"

namespace bpfls {

lsp::Diagnostic to_lsp(
    [[maybe_unused]] const bpftrace::ast::Diagnostic &diagnostic,
    [[maybe_unused]] lsp::DiagnosticSeverity severity)
{
    return {};
}

std::vector<lsp::Diagnostic> diagnostics([[maybe_unused]] Document &document)
{
    return {};
}

} // namespace bpfls
