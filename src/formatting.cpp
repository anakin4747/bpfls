#include "formatting.h"

#include <sstream>

#include "ast/passes/printer.h"

namespace bpfls {

std::optional<std::string> format(Document &document)
{
    auto &ast = document.ast();
    if (ast.root == nullptr || !ast.diagnostics().ok())
        return std::nullopt;

    std::ostringstream out;
    bpftrace::ast::Printer printer(ast, out, bpftrace::ast::FormatMode::Full);
    printer.visit(ast.root);
    return out.str();
}

} // namespace bpfls
