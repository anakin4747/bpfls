#pragma once

#include <vector>

#include <lsp/types.h>

#include "ast/diagnostic.h"
#include "document.h"

namespace bpfls {

// Converts a bpftrace diagnostic to LSP. The message is bpftrace's own,
// followed by any hints on their own lines as bpftrace prints them
// ("HINT: ...").
lsp::Diagnostic to_lsp(const bpftrace::ast::Diagnostic &diagnostic,
                       lsp::DiagnosticSeverity severity);

// All diagnostics bpftrace reported while parsing `document`, as LSP.
std::vector<lsp::Diagnostic> diagnostics(Document &document);

} // namespace bpfls
