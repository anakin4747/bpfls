#include "document.h"

#include <type_traits>
#include <utility>

#include "ast/visitor.h"
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

namespace {

// Walks the AST and remembers the innermost node whose source range contains
// a given position. Children are visited after their parents, so the last
// match is the innermost one.
class NodeFinder : public bpftrace::ast::Visitor<NodeFinder> {
public:
    // `line` and `column` are 1-based, as in bpftrace's source locations.
    NodeFinder(int line, int column) : line_(line), column_(column)
    {
    }

    // Catches every node type. There is deliberately no
    // `using Visitor::visit;`: the base class's non-template overloads would
    // then win overload resolution and bypass this method.
    template <typename T>
    void visit(T &node)
    {
        // The Program spans the whole file, and the parser gives every block an
        // implicit None value spanning the whole block; neither is a useful
        // answer.
        if constexpr (std::is_base_of_v<bpftrace::ast::Node, T> &&
                      !std::is_same_v<T, bpftrace::ast::Program> &&
                      !std::is_same_v<T, bpftrace::ast::None>) {
            if (contains(node.loc->current))
                found_ = &node;
        }
        Visitor<NodeFinder>::visit(node);
    }

    [[nodiscard]] bpftrace::ast::Node *found() const
    {
        return found_;
    }

private:
    // Ranges are half-open: `end` points just past the last character.
    [[nodiscard]] bool contains(const bpftrace::ast::SourceLocation &loc) const
    {
        auto before = [](int l1, int c1, int l2, int c2) {
            return l1 < l2 || (l1 == l2 && c1 < c2);
        };
        return !before(line_, column_, loc.begin.line, loc.begin.column) &&
               before(line_, column_, loc.end.line, loc.end.column);
    }

    int line_;
    int column_;
    bpftrace::ast::Node *found_ = nullptr;
};

} // namespace

bpftrace::ast::Node *Document::node_at(Position pos)
{
    if (ast_.root == nullptr)
        return nullptr;

    NodeFinder finder(static_cast<int>(pos.line) + 1,
                      static_cast<int>(pos.character) + 1);
    finder.visit(*ast_.root);
    return finder.found();
}

} // namespace bpfls
