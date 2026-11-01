#pragma once

#include <string>

#include "ast/ast.h"
#include "ast/context.h"

namespace bpfls {

// A bpftrace script open in the editor: its text plus the AST bpftrace's
// parser builds from it.
class Document {
public:
  Document(std::string uri, std::string text);

  const std::string &uri() const;
  const std::string &text() const;
  bpftrace::ast::ASTContext &ast();

private:
  std::string uri_;
  bpftrace::ast::ASTContext ast_;
};

} // namespace bpfls
