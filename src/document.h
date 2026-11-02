#pragma once

#include <string>

#include <lsp/types.h>

#include "ast/ast.h"
#include "ast/context.h"

namespace bpfls {

// A bpftrace script open in the editor: its text plus the AST bpftrace's
// parser builds from it, checked by bpftrace's deprecation pass.
class Document {
public:
    Document(std::string uri, std::string text);

    const std::string &uri() const;
    const std::string &text() const;
    bpftrace::ast::ASTContext &ast();
    // Whether the parser reported no errors. Later checks may still add
    // errors to the AST's diagnostics, but the AST is complete.
    bool parsed() const;

    // Returns the innermost AST node whose source range contains `pos`, or
    // nullptr if no node does. `pos` is 0-based, as in LSP.
    bpftrace::ast::Node *node_at(const lsp::Position &pos);

private:
    std::string uri_;
    bpftrace::ast::ASTContext ast_;
    bool parsed_ = false;
};

} // namespace bpfls
