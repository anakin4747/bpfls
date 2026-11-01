#pragma once

#include <string>

#include "ast/ast.h"
#include "ast/context.h"

namespace bpfls {

// A position in a document, following LSP conventions: 0-based line and
// 0-based character offset within that line.
struct Position {
  unsigned int line;
  unsigned int character;
};

// A bpftrace script open in the editor: its text plus the AST bpftrace's
// parser builds from it.
class Document {
public:
  Document(std::string uri, std::string text);

  const std::string &uri() const;
  const std::string &text() const;
  bpftrace::ast::ASTContext &ast();

  // Returns the innermost AST node whose source range contains `pos`, or
  // nullptr if no node does.
  bpftrace::ast::Node *node_at(Position pos);

private:
  std::string uri_;
  bpftrace::ast::ASTContext ast_;
};

} // namespace bpfls
