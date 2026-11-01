#include "document.h"

#include <utility>

#include "parser.h"

namespace bpfls {

Document::Document(std::string uri, std::string text)
    : uri_(std::move(uri)), ast_(uri_, std::move(text))
{
  ast_.root = bpftrace::Parser(ast_).parse();
}

const std::string &Document::uri() const
{
  return uri_;
}

const std::string &Document::text() const
{
  return ast_.source()->contents;
}

bpftrace::ast::ASTContext &Document::ast()
{
  return ast_;
}

} // namespace bpfls
