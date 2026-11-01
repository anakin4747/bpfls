#include "hover_docs.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>

#include "bpf_docs.h"
#include "probe_types.h"
#include "util/strings.h"

namespace bpfls {

namespace {

namespace ast = bpftrace::ast;

consteval std::string_view language(std::string_view start,
                                    std::string_view end)
{
    return bpf_docs("language.md", start, end);
}

// Each section runs up to the next heading of the same or a higher level.

// Probes.
constexpr auto begin_end_docs = language("### begin/end", "### test");
constexpr auto test_docs = language("### test", "### bench");
constexpr auto bench_docs = language("### bench", "### self");
constexpr auto self_docs = language("### self", "### hardware");
constexpr auto hardware_docs = language("### hardware", "### interval");
constexpr auto interval_docs = language("### interval", "### iterator");
constexpr auto iterator_docs = language("### iterator", "### fentry and fexit");
constexpr auto fentry_docs = language("### fentry and fexit",
                                      "### kprobe and kretprobe");
constexpr auto kprobe_docs = language("### kprobe and kretprobe",
                                      "### profile");
constexpr auto profile_docs = language("### profile", "### rawtracepoint");
constexpr auto rawtracepoint_docs = language("### rawtracepoint",
                                             "### software");
constexpr auto software_docs = language("### software", "### tracepoint");
constexpr auto tracepoint_docs = language("### tracepoint",
                                          "### uprobe, uretprobe");
constexpr auto uprobe_docs = language("### uprobe, uretprobe", "### usdt");
constexpr auto usdt_docs = language("### usdt", "### watchpoint");
constexpr auto watchpoint_docs = language("### watchpoint", "## Types");

// Config.
constexpr auto config_block_docs = language("## Config Block",
                                            "## Config Variables");

struct ConfigVariable {
    std::string_view name;
    std::string_view docs;
};

constexpr std::array config_variables = {
    ConfigVariable{ "cache_user_symbols",
                    language("### cache_user_symbols", "### cpp_demangle") },
    ConfigVariable{ "cpp_demangle",
                    language("### cpp_demangle", "### lazy_symbolication") },
    ConfigVariable{ "lazy_symbolication",
                    language("### lazy_symbolication", "### license") },
    ConfigVariable{ "license", language("### license", "### log_size") },
    ConfigVariable{ "log_size", language("### log_size", "### max_bpf_progs") },
    ConfigVariable{ "max_bpf_progs",
                    language("### max_bpf_progs", "### max_cat_bytes") },
    ConfigVariable{ "max_cat_bytes",
                    language("### max_cat_bytes", "### max_map_keys") },
    ConfigVariable{ "max_map_keys",
                    language("### max_map_keys", "### max_probes") },
    ConfigVariable{ "max_probes",
                    language("### max_probes", "### max_strlen") },
    ConfigVariable{ "max_strlen",
                    language("### max_strlen", "### missing_probes") },
    ConfigVariable{ "missing_probes",
                    language("### missing_probes", "### on_stack_limit") },
    ConfigVariable{ "on_stack_limit",
                    language("### on_stack_limit", "### perf_rb_pages") },
    ConfigVariable{ "perf_rb_pages",
                    language("### perf_rb_pages", "### show_debug_info") },
    ConfigVariable{ "show_debug_info",
                    language("### show_debug_info", "### stack_mode") },
    ConfigVariable{ "stack_mode",
                    language("### stack_mode", "### str_trunc_trailer") },
    ConfigVariable{
        "str_trunc_trailer",
        language("### str_trunc_trailer", "### print_maps_on_exit") },
    ConfigVariable{
        "print_maps_on_exit",
        language("### print_maps_on_exit", "### unstable features") },
};

constexpr auto unstable_features_docs = language("### unstable features",
                                                 "## Data Types");

// Language constructs.
constexpr auto args_docs = language("### args", "## Arrays");
constexpr auto arg_n_docs = language("### argN", "### args");
constexpr auto positional_docs = language("### Positional Parameters",
                                          "## Comments");
constexpr auto conditionals_docs = language("## Conditionals",
                                            "## Config Block");
constexpr auto imports_docs = language("## Imports", "## Kernel Functions");
constexpr auto for_docs = language("### For", "### While");
constexpr auto while_docs = language("### While", "### Unroll");
constexpr auto unroll_docs = language("### Unroll", "## Macros");
constexpr auto macros_docs = language("## Macros",
                                      "## Operators and Expressions");
constexpr auto increment_docs =
    language("### Increment and Decrement Operators", "### Block Expressions");
constexpr auto type_context_docs = language("### Type Context Builtins",
                                            "### Pointers");
constexpr auto tuples_docs = language("### Tuples", "### Records");
constexpr auto records_docs = language("### Records", "### Type Conversion");
constexpr auto type_conversion_docs = language("### Type Conversion",
                                               "## Variables and Maps");
constexpr auto map_declarations_docs =
    language("### Map Declarations", "### Maps without Explicit Keys");

// The docs for an attach point's probe type. The provider is resolved the
// way bpftrace's attach point pass does: the text before the first ':' or
// '@', with aliases expanded and case ignored (so `BEGIN` is `begin`).
// Wildcard providers match no single type and get no docs.
std::string_view probe_docs(const ast::AttachPoint &ap)
{
    std::string_view raw = ap.raw_input;
    std::string provider = bpftrace::expand_probe_name(
        std::string(raw.substr(0, raw.find_first_of(":@"))));

    switch (bpftrace::probetype(provider)) {
        case bpftrace::ProbeType::special:
            return provider == "self" ? self_docs : begin_end_docs;
        case bpftrace::ProbeType::test:
            return test_docs;
        case bpftrace::ProbeType::benchmark:
            return bench_docs;
        case bpftrace::ProbeType::kprobe:
        case bpftrace::ProbeType::kretprobe:
            return kprobe_docs;
        case bpftrace::ProbeType::uprobe:
        case bpftrace::ProbeType::uretprobe:
            return uprobe_docs;
        case bpftrace::ProbeType::usdt:
            return usdt_docs;
        case bpftrace::ProbeType::tracepoint:
            return tracepoint_docs;
        case bpftrace::ProbeType::profile:
            return profile_docs;
        case bpftrace::ProbeType::interval:
            return interval_docs;
        case bpftrace::ProbeType::software:
            return software_docs;
        case bpftrace::ProbeType::hardware:
            return hardware_docs;
        case bpftrace::ProbeType::watchpoint:
            return watchpoint_docs;
        case bpftrace::ProbeType::fentry:
        case bpftrace::ProbeType::fexit:
            return fentry_docs;
        case bpftrace::ProbeType::iter:
            return iterator_docs;
        case bpftrace::ProbeType::rawtracepoint:
            return rawtracepoint_docs;
        case bpftrace::ProbeType::invalid:
            return {};
    }
    return {};
}

// The docs for a config variable. Its name is normalized the way bpftrace's
// config does: case is ignored and a `bpftrace_` prefix (the environment
// variable spelling) is dropped.
std::string_view config_docs(const ast::AssignConfigVarStatement &assignment)
{
    std::string name = bpftrace::util::to_lower(assignment.var);
    if (name.starts_with("bpftrace_"))
        name.erase(0, std::string_view("bpftrace_").size());

    if (name.starts_with("unstable_"))
        return unstable_features_docs;
    for (const auto &variable : config_variables) {
        if (variable.name == name)
            return variable.docs;
    }
    return {};
}

// Whether `ident` is `argN`: `arg` followed by digits.
bool is_arg_n(std::string_view ident)
{
    return ident.size() > 3 && ident.starts_with("arg") &&
           std::ranges::all_of(ident.substr(3), [](char c) {
        return std::isdigit(static_cast<unsigned char>(c)) != 0;
    });
}

std::string_view builtin_docs(const ast::Builtin &builtin)
{
    if (builtin.ident == "args")
        return args_docs;
    if (is_arg_n(builtin.ident))
        return arg_n_docs;
    return {};
}

std::string_view unop_docs(const ast::Unop &unop)
{
    switch (unop.op) {
        case ast::Operator::PRE_INCREMENT:
        case ast::Operator::PRE_DECREMENT:
        case ast::Operator::POST_INCREMENT:
        case ast::Operator::POST_DECREMENT:
            return increment_docs;
        default:
            return {};
    }
}

} // namespace

std::string_view hover_docs(const ast::Node *node)
{
    if (node == nullptr)
        return {};

    if (const auto *ap = dynamic_cast<const ast::AttachPoint *>(node))
        return probe_docs(*ap);
    if (dynamic_cast<const ast::Config *>(node))
        return config_block_docs;
    if (const auto *a = dynamic_cast<const ast::AssignConfigVarStatement *>(
            node))
        return config_docs(*a);
    if (const auto *b = dynamic_cast<const ast::Builtin *>(node))
        return builtin_docs(*b);
    if (dynamic_cast<const ast::PositionalParameter *>(node) ||
        dynamic_cast<const ast::PositionalParameterCount *>(node))
        return positional_docs;
    // A for loop's Range starts at the `for` keyword, so hovering `for` in
    // `for $i : 0..3` lands on it rather than on the For.
    if (dynamic_cast<const ast::For *>(node) ||
        dynamic_cast<const ast::Range *>(node))
        return for_docs;
    if (dynamic_cast<const ast::While *>(node))
        return while_docs;
    if (dynamic_cast<const ast::Unroll *>(node))
        return unroll_docs;
    // Both if/else and the ternary operator.
    if (dynamic_cast<const ast::IfExpr *>(node))
        return conditionals_docs;
    if (dynamic_cast<const ast::RootImport *>(node) ||
        dynamic_cast<const ast::StatementImport *>(node))
        return imports_docs;
    if (dynamic_cast<const ast::Macro *>(node))
        return macros_docs;
    if (dynamic_cast<const ast::MapDeclStatement *>(node))
        return map_declarations_docs;
    if (dynamic_cast<const ast::Tuple *>(node) ||
        dynamic_cast<const ast::TupleAccess *>(node))
        return tuples_docs;
    if (dynamic_cast<const ast::Record *>(node))
        return records_docs;
    if (dynamic_cast<const ast::Cast *>(node))
        return type_conversion_docs;
    if (const auto *u = dynamic_cast<const ast::Unop *>(node))
        return unop_docs(*u);
    // Typeof is not included: the parser also uses it for the type inside
    // casts, sizeof and offsetof, while `typeof(...)` in an expression is
    // parsed as a call.
    if (dynamic_cast<const ast::Sizeof *>(node) ||
        dynamic_cast<const ast::Offsetof *>(node) ||
        dynamic_cast<const ast::Typeinfo *>(node))
        return type_context_docs;
    if (const auto *call = dynamic_cast<const ast::Call *>(node))
        return call->func == "typeof" ? type_context_docs : std::string_view{};
    return {};
}

} // namespace bpfls
