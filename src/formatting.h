#pragma once

#include <optional>
#include <string>

#include "document.h"

namespace bpfls {

// Formats `document` exactly like `bpftrace --fmt`, using bpftrace's own
// printer. Returns nullopt if the script has syntax errors: the parser
// recovers from them, but the AST it leaves behind is missing the broken
// parts, so printing it would delete code.
std::optional<std::string> format(Document &document);

} // namespace bpfls
