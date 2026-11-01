#pragma once

#include <string>

#include "ast/ast.h"

namespace bpfls {

// The documentation to show when hovering `node`: its section of bpftrace's
// language.md or stdlib.md, built into the binary at compile time, or both,
// each labelled with its file, for names documented in both. Empty if the
// node has no section of its own.
std::string hover_docs(const bpftrace::ast::Node *node);

} // namespace bpfls
