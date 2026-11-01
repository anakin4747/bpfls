#pragma once

#include <string_view>

#include "ast/ast.h"

namespace bpfls {

// The documentation to show when hovering `node`: its section of bpftrace's
// language.md, built into the binary at compile time. Empty if the node has
// no section of its own.
std::string_view hover_docs(const bpftrace::ast::Node *node);

} // namespace bpfls
