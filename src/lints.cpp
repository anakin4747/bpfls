#include "lints.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include "ast/visitor.h"
#include "config.h"
#include "probe_types.h"

namespace bpfls {

namespace {

namespace ast = bpftrace::ast;

// Normalizes a config key the way bpftrace's Config does: case is ignored
// and a `bpftrace_` prefix (as in the environment variable) is dropped.
std::string normalized_config_key(std::string_view key)
{
    std::string result;
    for (char c : key)
        result += static_cast<char>(
            std::tolower(static_cast<unsigned char>(c)));
    constexpr std::string_view prefix = "bpftrace_";
    if (result.starts_with(prefix))
        result.erase(0, prefix.size());
    return result;
}

std::size_t edit_distance(std::string_view a, std::string_view b)
{
    std::vector<std::size_t> row(b.size() + 1);
    for (std::size_t j = 0; j <= b.size(); ++j)
        row[j] = j;
    for (std::size_t i = 1; i <= a.size(); ++i) {
        std::size_t diagonal = row[0];
        row[0] = i;
        for (std::size_t j = 1; j <= b.size(); ++j) {
            std::size_t above = row[j];
            row[j] = std::min({ row[j] + 1,
                                row[j - 1] + 1,
                                diagonal + (a[i - 1] == b[j - 1] ? 0 : 1) });
            diagonal = above;
        }
    }
    return row[b.size()];
}

bool is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\r';
}

// The document's text, with conversions between bpftrace's 1-based
// positions, byte offsets and LSP positions.
// TODO: like diagnostics.cpp, LSP characters are taken to be bytes, so
// positions are only right for ASCII text.
class Text {
public:
    explicit Text(std::string_view text) : text_(text)
    {
        line_starts_.push_back(0);
        for (std::size_t i = 0; i < text.size(); ++i) {
            if (text[i] == '\n')
                line_starts_.push_back(i + 1);
        }
    }

    [[nodiscard]] std::size_t offset(
        const ast::SourceLocation::Position &pos) const
    {
        auto line = static_cast<std::size_t>(std::max(pos.line, 1) - 1);
        line = std::min(line, line_starts_.size() - 1);
        auto column = static_cast<std::size_t>(std::max(pos.column, 1) - 1);
        return std::min(line_starts_[line] + column, text_.size());
    }

    [[nodiscard]] std::size_t begin(const ast::Node &node) const
    {
        return offset(node.loc->current.begin);
    }

    [[nodiscard]] std::size_t end(const ast::Node &node) const
    {
        return std::max(begin(node), offset(node.loc->current.end));
    }

    [[nodiscard]] lsp::Position position(std::size_t offset) const
    {
        auto it = std::upper_bound(line_starts_.begin(),
                                   line_starts_.end(),
                                   offset);
        auto line = static_cast<std::size_t>(it - line_starts_.begin()) - 1;
        return {
            .line = static_cast<lsp::Uint>(line),
            .character = static_cast<lsp::Uint>(offset - line_starts_[line]),
        };
    }

    [[nodiscard]] lsp::Range range(std::size_t begin, std::size_t end) const
    {
        return { .start = position(begin), .end = position(end) };
    }

    [[nodiscard]] lsp::Range range(const ast::Node &node) const
    {
        return range(begin(node), end(node));
    }

    [[nodiscard]] std::string_view slice(std::size_t begin,
                                         std::size_t end) const
    {
        return text_.substr(begin, end - begin);
    }

    [[nodiscard]] std::string_view of(const ast::Node &node) const
    {
        return slice(begin(node), end(node));
    }

    [[nodiscard]] char at(std::size_t offset) const
    {
        return offset < text_.size() ? text_[offset] : '\0';
    }

    // The offset of the start of the line containing `offset`.
    [[nodiscard]] std::size_t line_start(std::size_t offset) const
    {
        auto it = std::upper_bound(line_starts_.begin(),
                                   line_starts_.end(),
                                   offset);
        return *(it - 1);
    }

    // The range to delete to remove the statement at [begin, end) along with
    // its `;`. A statement alone on its line takes the whole line with it.
    [[nodiscard]] lsp::Range statement_removal(std::size_t begin,
                                               std::size_t end) const
    {
        while (is_space(at(end)))
            ++end;
        if (at(end) == ';')
            ++end;

        std::size_t ls = line_start(begin);
        bool alone_before = std::all_of(text_.begin() + ls,
                                        text_.begin() + begin,
                                        is_space);
        std::size_t after = end;
        while (is_space(at(after)))
            ++after;
        if (alone_before && (at(after) == '\n' || after >= text_.size())) {
            if (at(after) == '\n')
                ++after;
            return range(ls, after);
        }
        return range(begin, after);
    }

private:
    std::string_view text_;
    std::vector<std::size_t> line_starts_;
};

// Walks the AST and collects lints.
class Linter : public ast::Visitor<Linter> {
public:
    explicit Linter(const Text &text) : text_(text)
    {
    }

    using Visitor<Linter>::visit;

    std::vector<Lint> take()
    {
        return std::move(lints_);
    }

    // bpftrace's deprecation pass reports while loops themselves.
    void visit(ast::While &loop)
    {
        ++loop_depth_;
        Visitor<Linter>::visit(loop);
        --loop_depth_;
    }

    void visit(ast::For &loop)
    {
        ++loop_depth_;
        Visitor<Linter>::visit(loop);
        --loop_depth_;
    }

    void visit(ast::AttachPoint &ap)
    {
        std::string_view raw = ap.raw_input;
        std::string_view provider = raw.substr(0, raw.find_first_of(":@"));
        std::string lower;
        for (char c : provider)
            lower += static_cast<char>(
                std::tolower(static_cast<unsigned char>(c)));

        // bpftrace silently expands these aliases; other aliases (`f`, `k`,
        // ...) are deliberate shorthands and are left alone.
        if (lower != "kfunc" && lower != "kretfunc")
            return;
        std::string replacement = bpftrace::expand_probe_name(lower);

        // The attach point may be named (`name=kfunc:f`), so look for the
        // provider rather than assuming it starts the node.
        std::string_view node_text = text_.of(ap);
        auto pos = node_text.find(std::string(provider) + ":");
        if (pos == std::string_view::npos)
            return;
        std::size_t begin = text_.begin(ap) + pos;
        auto range = text_.range(begin, begin + provider.size());
        add(range,
            lsp::DiagnosticSeverity::Hint,
            "deprecated-probe-type",
            std::string(provider) + " is a deprecated alias of " + replacement +
                "\nHINT: use " + replacement + " instead",
            { lsp::DiagnosticTag::Deprecated },
            { replace("Replace with " + replacement, range, replacement) });
    }

    void visit(ast::Call &call)
    {
        if (call.ns == ast::Call::Namespace::Default) {
            if (call.func == "sym")
                removed_sym(call);
            else if (call.func == "delete")
                deprecated_delete(call);
            else if (call.func == "exit" && loop_depth_ > 0)
                add(text_.range(call),
                    lsp::DiagnosticSeverity::Error,
                    "exit-in-loop",
                    "exit() is not allowed inside a loop\n"
                    "HINT: move it out of the loop, e.g. by setting a "
                    "variable and breaking out",
                    {});
        }
        Visitor<Linter>::visit(call);
    }

    void visit(ast::Identifier &identifier)
    {
        const std::string &name = identifier.ident;
        if (name == "stack") {
            auto range = text_.range(identifier);
            add(range,
                lsp::DiagnosticSeverity::Error,
                "removed-stack",
                "stack was removed\nHINT: use kstack instead",
                {},
                { replace("Replace with kstack", range, "kstack") });
        }
        Visitor<Linter>::visit(identifier);
    }

    void visit(ast::FieldAccess &access)
    {
        field_access(access);
        Visitor<Linter>::visit(access);
    }

    void visit(ast::AssignConfigVarStatement &assignment)
    {
        config_key(assignment);
    }

    void visit(ast::PositionalParameter &param)
    {
        positional(param);
    }

    void visit(ast::PositionalParameterCount &param)
    {
        positional(param);
    }

    void visit(ast::Binop &binop)
    {
        if (binop.op == ast::Operator::EQ || binop.op == ast::Operator::NE) {
            bool left_call = is_strncmp(binop.left) && is_zero(binop.right);
            bool right_call = is_strncmp(binop.right) && is_zero(binop.left);
            if (left_call || right_call)
                strncmp_equality(binop);
        }
        Visitor<Linter>::visit(binop);
    }

    void visit(ast::Unop &unop)
    {
        if (unop.op == ast::Operator::LNOT && is_strncmp(unop.expr))
            strncmp_equality(unop);
        Visitor<Linter>::visit(unop);
    }

    void visit(ast::IfExpr &if_expr)
    {
        has_key_then_read(if_expr);
        Visitor<Linter>::visit(if_expr);
    }

private:
    void add(lsp::Range range,
             lsp::DiagnosticSeverity severity,
             std::string code,
             std::string message,
             std::vector<lsp::DiagnosticTag> tags,
             std::vector<Fix> fixes = {})
    {
        lsp::Diagnostic diagnostic{
            .range = range,
            .message = std::move(message),
            .severity = severity,
            .code = std::move(code),
            .source = "bpfls",
        };
        if (!tags.empty()) {
            diagnostic.tags.emplace();
            for (auto tag : tags)
                diagnostic.tags->push_back(tag);
        }
        lints_.push_back({ std::move(diagnostic), std::move(fixes) });
    }

    static Fix replace(std::string title, lsp::Range range, std::string text)
    {
        return { std::move(title),
                 { lsp::TextEdit{ .range = range,
                                  .newText = std::move(text) } } };
    }

    void removed_sym(ast::Call &call)
    {
        std::size_t begin = text_.begin(call);
        if (!text_.of(call).starts_with("sym"))
            return;
        auto range = text_.range(begin, begin + 3);
        add(range,
            lsp::DiagnosticSeverity::Error,
            "removed-sym",
            "sym() was removed\nHINT: use ksym() instead",
            {},
            { replace("Replace with ksym", range, "ksym") });
    }

    // `@m[a]` as `@m, a` and `@m[a, b]` as `@m, (a, b)`, the arguments
    // delete() takes now. Nullopt if the source text is not as expected.
    std::optional<std::string> delete_args(ast::MapAccess &access) const
    {
        std::string_view whole = text_.of(access);
        std::string_view map = text_.of(*access.map);
        auto open = whole.find('[', map.size());
        auto close = whole.rfind(']');
        if (!whole.starts_with(map) || open == std::string_view::npos ||
            close == std::string_view::npos || close < open)
            return std::nullopt;
        std::string key(whole.substr(open + 1, close - open - 1));
        // Several keys parse to a tuple spanning the whole access; a tuple
        // written out in brackets (`@m[(a, b)]`) has its own location.
        auto *tuple = access.key.as<ast::Tuple>();
        if (tuple != nullptr && text_.begin(*tuple) == text_.begin(access) &&
            text_.end(*tuple) == text_.end(access))
            key = "(" + key + ")";
        return std::string(map) + ", " + key;
    }

    void deprecated_delete(ast::Call &call)
    {
        std::vector<ast::MapAccess *> accesses;
        for (auto &arg : call.vargs) {
            auto *access = arg.as<ast::MapAccess>();
            if (access == nullptr)
                return;
            accesses.push_back(access);
        }
        if (accesses.empty())
            return;

        std::vector<std::string> calls;
        for (auto *access : accesses) {
            auto args = delete_args(*access);
            if (!args.has_value()) {
                calls.clear();
                break;
            }
            calls.push_back("delete(" + *args + ")");
        }

        auto range = text_.range(call);
        if (accesses.size() == 1) {
            std::vector<Fix> fixes;
            if (!calls.empty())
                fixes.push_back(replace("Replace with " + calls.front(),
                                        range,
                                        calls.front()));
            add(range,
                lsp::DiagnosticSeverity::Hint,
                "deprecated-delete",
                "delete(@map[key]) is deprecated\n"
                "HINT: pass the map and the key separately: "
                "delete(@map, key)",
                { lsp::DiagnosticTag::Deprecated },
                std::move(fixes));
            return;
        }

        std::vector<Fix> fixes;
        if (!calls.empty()) {
            // Put each call on its own line at the original indentation, or
            // on the same line if the call does not start its line.
            std::size_t begin = text_.begin(call);
            std::size_t ls = text_.line_start(begin);
            std::string_view indent = text_.slice(ls, begin);
            bool own_line = std::all_of(indent.begin(), indent.end(), is_space);
            std::string separator = own_line ? ";\n" + std::string(indent)
                                             : std::string("; ");
            std::string replacement = calls.front();
            for (std::size_t i = 1; i < calls.size(); ++i)
                replacement += separator + calls[i];
            fixes.push_back(replace("Split into one delete() per key",
                                    range,
                                    std::move(replacement)));
        }
        add(range,
            lsp::DiagnosticSeverity::Error,
            "removed-multi-key-delete",
            "delete() no longer takes several map elements\n"
            "HINT: call delete(@map, key) once for each key",
            {},
            std::move(fixes));
    }

    void field_access(ast::FieldAccess &access)
    {
        std::string_view whole = text_.of(access);
        const std::string &field = access.field;
        if (field.empty() || !whole.ends_with(field))
            return;

        // Find the operator before the field name.
        std::size_t pos = whole.size() - field.size();
        while (pos > 0 && (is_space(whole[pos - 1]) || whole[pos - 1] == '\n'))
            --pos;
        bool arrow = pos >= 2 && whole.substr(pos - 2, 2) == "->";
        bool dot = !arrow && pos >= 1 && whole[pos - 1] == '.';
        if (!arrow && !dot)
            return;
        std::size_t op_begin = text_.begin(access) + pos - (arrow ? 2 : 1);
        std::size_t end = text_.end(access);

        constexpr std::string_view data_loc = "data_loc_";
        auto *builtin = access.expr.as<ast::Builtin>();
        if (builtin != nullptr && builtin->ident == "args" &&
            field.starts_with(data_loc) && field.size() > data_loc.size()) {
            std::string name = field.substr(data_loc.size());
            add(text_.range(op_begin, end),
                lsp::DiagnosticSeverity::Error,
                "removed-data-loc",
                "tracepoint __data_loc fields are no longer prefixed with "
                "data_loc_\nHINT: use args." +
                    name + " instead",
                {},
                { replace("Replace with ." + name,
                          text_.range(op_begin, end),
                          "." + name) });
            return;
        }

        if (arrow) {
            auto range = text_.range(op_begin, op_begin + 2);
            add(range,
                lsp::DiagnosticSeverity::Hint,
                "legacy-arrow",
                "-> is legacy field access syntax\nHINT: use . instead",
                { lsp::DiagnosticTag::Deprecated },
                { replace("Replace -> with .", range, ".") });
        }
    }

    void config_key(ast::AssignConfigVarStatement &assignment)
    {
        const std::string &var = assignment.var;
        std::size_t begin = text_.begin(assignment);
        std::size_t end = text_.end(assignment);
        if (!text_.of(assignment).starts_with(var))
            return;
        auto key_range = text_.range(begin, begin + var.size());
        auto removal = text_.statement_removal(begin, end);

        // bpftrace reports keys that are deprecated but have no effect
        // before setting anything.
        if (bpftrace::DEPRECATED_CONFIGS.contains(var)) {
            add(key_range,
                lsp::DiagnosticSeverity::Warning,
                "deprecated-config",
                var + " is deprecated and has no effect",
                { lsp::DiagnosticTag::Deprecated,
                  lsp::DiagnosticTag::Unnecessary },
                { replace("Remove " + var, removal, "") });
            return;
        }

        // Everything else is up to bpftrace's Config, exactly as its config
        // analyser does it.
        bpftrace::Config config;
        auto result = std::visit([&](const auto &value) {
            return config.set(var, value);
        }, assignment.value);
        if (result)
            return;

        std::optional<std::string> new_name;
        std::string message;
        llvm::handleAllErrors(
            result.takeError(), [&](const bpftrace::RenameError &e) {
            new_name = e.new_name();
        }, [&](const llvm::ErrorInfoBase &e) { message = e.message(); });

        if (new_name.has_value()) {
            add(key_range,
                lsp::DiagnosticSeverity::Warning,
                "renamed-config",
                var + " has been renamed, please use " + *new_name,
                { lsp::DiagnosticTag::Deprecated },
                { replace("Rename to " + *new_name, key_range, *new_name) });
            return;
        }

        std::string key = normalized_config_key(var);
        auto keys = bpftrace::Config::keys();
        if (std::ranges::contains(keys, key)) {
            // A known key with a value bpftrace rejects.
            add(text_.range(begin, end),
                lsp::DiagnosticSeverity::Error,
                "invalid-config-value",
                message,
                {});
            return;
        }

        if (message.ends_with("can only be set as an environment variable")) {
            std::string env = "BPFTRACE_";
            for (char c : key)
                env += static_cast<char>(
                    std::toupper(static_cast<unsigned char>(c)));
            add(key_range,
                lsp::DiagnosticSeverity::Error,
                "env-only-config",
                message + "\nHINT: set " + env + " in the environment instead",
                {},
                { replace("Remove " + var, removal, "") });
            return;
        }

        // Unknown: suggest known keys a few typos away, closest first.
        std::vector<std::pair<std::size_t, std::string>> candidates;
        for (auto &known : keys) {
            auto distance = edit_distance(key, known);
            if (distance <= 2)
                candidates.emplace_back(distance, std::move(known));
        }
        std::ranges::stable_sort(candidates, {}, [](const auto &c) {
            return c.first;
        });
        std::vector<Fix> fixes;
        for (const auto &[distance, known] : candidates)
            fixes.push_back(
                replace("Did you mean " + known + "?", key_range, known));
        add(key_range,
            lsp::DiagnosticSeverity::Error,
            "unknown-config",
            message,
            {},
            std::move(fixes));
    }

    void positional(const ast::Node &param)
    {
        add(text_.range(param),
            lsp::DiagnosticSeverity::Hint,
            "positional-parameter",
            "consider named parameters instead of positional ones\n"
            "HINT: getopt(\"name\", default) gives a parameter a name, a "
            "type and a default value",
            {});
    }

    static bool is_strncmp(ast::Expression &expr)
    {
        auto *call = expr.as<ast::Call>();
        return call != nullptr && call->func == "strncmp" &&
               call->ns == ast::Call::Namespace::Default;
    }

    static bool is_zero(ast::Expression &expr)
    {
        auto *integer = expr.as<ast::Integer>();
        return integer != nullptr && integer->value == 0;
    }

    void strncmp_equality(const ast::Node &node)
    {
        add(text_.range(node),
            lsp::DiagnosticSeverity::Hint,
            "strncmp-equality",
            "strncmp() is only used to test for equality\n"
            "HINT: == and != compare strings directly; note they compare "
            "whole strings, not just the first n characters",
            {});
    }

    // Finds reads of `@map[key]` in a subtree; writes to it are not reads.
    class MapReadFinder : public ast::Visitor<MapReadFinder> {
    public:
        MapReadFinder(const std::string &map, ast::Expression &key)
            : map_(map), key_(key)
        {
        }

        using Visitor<MapReadFinder>::visit;

        void visit(ast::MapAccess &access)
        {
            if (access.map != nullptr && access.map->ident == map_ &&
                access.key == key_)
                found_ = true;
            Visitor<MapReadFinder>::visit(access);
        }

        void visit(ast::AssignMapStatement &assignment)
        {
            if (assignment.map_access != nullptr)
                visit(assignment.map_access->key);
            visit(assignment.expr);
        }

        [[nodiscard]] bool found() const
        {
            return found_;
        }

    private:
        const std::string &map_;
        ast::Expression &key_;
        bool found_ = false;
    };

    void has_key_then_read(ast::IfExpr &if_expr)
    {
        auto *call = if_expr.cond.as<ast::Call>();
        if (call == nullptr || call->func != "has_key" ||
            call->vargs.size() != 2)
            return;
        auto *map = call->vargs[0].as<ast::Map>();
        if (map == nullptr)
            return;
        MapReadFinder finder(map->ident, call->vargs[1]);
        finder.visit(if_expr.left);
        if (!finder.found())
            return;
        add(text_.range(*call),
            lsp::DiagnosticSeverity::Hint,
            "has-key-then-read",
            "has_key() followed by reading the same key looks the map up "
            "twice\nHINT: find(" +
                map->ident +
                ", key, $value) checks for the key and reads "
                "its value in a single lookup",
            {});
    }

    const Text &text_;
    std::vector<Lint> lints_;
    int loop_depth_ = 0;
};

} // namespace

std::vector<Lint> lint(Document &document)
{
    auto *root = document.ast().root;
    if (root == nullptr)
        return {};
    Text text(document.text());
    Linter linter(text);
    linter.visit(*root);
    return linter.take();
}

} // namespace bpfls
