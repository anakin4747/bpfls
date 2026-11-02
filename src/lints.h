#pragma once

#include <string>
#include <vector>

#include <lsp/types.h>

#include "document.h"

namespace bpfls {

// A quick fix for a lint: edits to the lint's document that resolve it.
struct Fix {
    std::string title;
    std::vector<lsp::TextEdit> edits;
};

// A problem bpfls finds in a script that bpftrace's parser does not report:
// deprecated or removed constructs, invalid config keys and discouraged
// patterns. Most of these bpftrace itself only reports in its later passes,
// which bpfls does not run.
struct Lint {
    lsp::Diagnostic diagnostic;
    // Empty when there is no fix that is safe to apply automatically.
    std::vector<Fix> fixes;
};

// Every lint in `document`, in source order of the checks' AST walk.
std::vector<Lint> lint(Document &document);

} // namespace bpfls
