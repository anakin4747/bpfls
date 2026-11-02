#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

// cppspec defines macros such as `it`, which clash with identifiers in
// bpftrace's headers, so project headers must be included first.
#include "lints.h"

#include "cppspec.hpp"

namespace {

std::vector<bpfls::Lint> lints_of(const std::string &text)
{
    bpfls::Document document("file:///t.bt", text);
    return bpfls::lint(document);
}

std::vector<std::string> codes_of(const std::string &text)
{
    std::vector<std::string> codes;
    for (const auto &lint : lints_of(text)) {
        const auto &code = lint.diagnostic.code;
        const auto *s = code.has_value() ? std::get_if<std::string>(&*code)
                                         : nullptr;
        codes.push_back(s != nullptr ? *s : "<no code>");
    }
    return codes;
}

// The only lint in `text` with `code`, if there is exactly one.
std::optional<bpfls::Lint> lint_with(const std::string &text,
                                     const std::string &code)
{
    std::optional<bpfls::Lint> found;
    for (auto &lint : lints_of(text)) {
        const auto *s = std::get_if<std::string>(&*lint.diagnostic.code);
        if (s == nullptr || *s != code)
            continue;
        if (found.has_value())
            return std::nullopt;
        found = std::move(lint);
    }
    return found;
}

std::size_t offset_of(const std::string &text, const lsp::Position &pos)
{
    std::size_t offset = 0;
    for (unsigned line = 0; line < pos.line; ++line)
        offset = text.find('\n', offset) + 1;
    return offset + pos.character;
}

// `text` with `fix` applied. Edits are applied back to front so earlier
// offsets stay valid.
std::string apply(std::string text, const bpfls::Fix &fix)
{
    auto edits = fix.edits;
    std::ranges::sort(edits, [&](const auto &a, const auto &b) {
        return offset_of(text, a.range.start) > offset_of(text, b.range.start);
    });
    for (const auto &edit : edits) {
        auto begin = offset_of(text, edit.range.start);
        auto end = offset_of(text, edit.range.end);
        text.replace(begin, end - begin, edit.newText);
    }
    return text;
}

// A lint the linter must find: `text` has exactly one lint with `code`, of
// `severity`, and applying its first fix gives `fixed` (nullopt: no fix).
struct Case {
    std::string text;
    std::string code;
    lsp::DiagnosticSeverity severity;
    std::optional<std::string> fixed;
};

using S = lsp::DiagnosticSeverity;

// clang-format off
const std::vector<Case> cases = {
    // Deprecated syntax.
    { "kfunc:f { }", "deprecated-probe-type", S::Hint, "fentry:f { }" },
    { "kretfunc:f { }", "deprecated-probe-type", S::Hint, "fexit:f { }" },
    { "begin { }\nkprobe:a, kfunc:f { }", "deprecated-probe-type", S::Hint, "begin { }\nkprobe:a, fentry:f { }" },
    { "begin { delete(@m[1]); }", "deprecated-delete", S::Hint, "begin { delete(@m, 1); }" },
    { "begin { delete(@m[1, \"a\"]); }", "deprecated-delete", S::Hint, "begin { delete(@m, (1, \"a\")); }" },
    { "begin { delete(@m[(1, 2)]); }", "deprecated-delete", S::Hint, "begin { delete(@m, (1, 2)); }" },
    { "begin { $x = $p->field; }", "legacy-arrow", S::Hint, "begin { $x = $p.field; }" },
    // Config keys.
    { "config = { symbol_source = 1 }\nbegin { }", "deprecated-config", S::Warning, "config = { }\nbegin { }" },
    { "config = {\n  unstable_macro = 1;\n  max_strlen = 2;\n}\nbegin { }", "deprecated-config", S::Warning, "config = {\n  max_strlen = 2;\n}\nbegin { }" },
    { "config = { unstable_addr = 1; max_strlen = 2 }\nbegin { }", "deprecated-config", S::Warning, "config = { max_strlen = 2 }\nbegin { }" },
    { "config = { strlen = 64 }\nbegin { }", "renamed-config", S::Warning, "config = { max_strlen = 64 }\nbegin { }" },
    { "config = { no_cpp_demangle = 1 }\nbegin { }", "renamed-config", S::Warning, "config = { cpp_demangle = 1 }\nbegin { }" },
    { "config = { cat_bytes_max = 1 }\nbegin { }", "renamed-config", S::Warning, "config = { max_cat_bytes = 1 }\nbegin { }" },
    { "config = { map_keys_max = 1 }\nbegin { }", "renamed-config", S::Warning, "config = { max_map_keys = 1 }\nbegin { }" },
    { "config = {\n  btf = \"x\";\n}\nbegin { }", "env-only-config", S::Error, "config = {\n}\nbegin { }" },
    { "config = { max_strlenn = 1 }\nbegin { }", "unknown-config", S::Error, "config = { max_strlen = 1 }\nbegin { }" },
    { "config = { nonsense = 1 }\nbegin { }", "unknown-config", S::Error, std::nullopt },
    { "config = { stack_mode = nonsense }\nbegin { }", "invalid-config-value", S::Error, std::nullopt },
    { "config = { max_strlen = \"x\" }\nbegin { }", "invalid-config-value", S::Error, std::nullopt },
    // Removed constructs.
    { "begin { print(sym(1)); }", "removed-sym", S::Error, "begin { print(ksym(1)); }" },
    { "kprobe:f { @[stack] = count(); }", "removed-stack", S::Error, "kprobe:f { @[kstack] = count(); }" },
    { "tracepoint:a:b { $x = args->data_loc_name; }", "removed-data-loc", S::Error, "tracepoint:a:b { $x = args.name; }" },
    { "tracepoint:a:b { $x = args.data_loc_name; }", "removed-data-loc", S::Error, "tracepoint:a:b { $x = args.name; }" },
    { "begin {\n  delete(@m[1], @m[2]);\n}", "removed-multi-key-delete", S::Error, "begin {\n  delete(@m, 1);\n  delete(@m, 2);\n}" },
    { "begin { $a = 1; delete(@m[1], @n[2]); }", "removed-multi-key-delete", S::Error, "begin { $a = 1; delete(@m, 1); delete(@n, 2); }" },
    { "begin { for ($i : 0..3) { exit(); } }", "exit-in-loop", S::Error, std::nullopt },
    { "begin { for ($kv : @m) { if (1) { exit(); } } }", "exit-in-loop", S::Error, std::nullopt },
    // Soft recommendations.
    { "begin { if (strncmp($a, $b, 3) == 0) { } }", "strncmp-equality", S::Hint, std::nullopt },
    { "begin { if (0 != strncmp($a, $b, 3)) { } }", "strncmp-equality", S::Hint, std::nullopt },
    { "begin { if (!strncmp($a, $b, 3)) { } }", "strncmp-equality", S::Hint, std::nullopt },
    { "begin { if (has_key(@m, 1)) { print(@m[1]); } }", "has-key-then-read", S::Hint, std::nullopt },
    { "begin { print($1); }", "positional-parameter", S::Hint, std::nullopt },
    { "begin { print($#); }", "positional-parameter", S::Hint, std::nullopt },
};

// Scripts with nothing to lint, including near misses of the cases above.
const std::vector<std::string> clean = {
    "begin { print(1); }",
    "fentry:f { } fexit:f { }",
    "begin { delete(@m, 1); delete(@m); }",
    "begin { $x = $p.field; }",
    "config = { max_strlen = 1; BPFTRACE_MAX_PROBES = 2; unstable_tseries = enable }\nbegin { }",
    "kprobe:f { print(ksym(1)); @[kstack] = count(); }",
    "tracepoint:a:b { $x = args.name; }",
    "begin { exit(); for ($i : 0..3) { } }",
    "begin { while (1) { } }", // bpftrace's deprecation pass reports this
    "uprobe:/b:f { $x = sarg0; }", // and this
    "begin { unroll(3) { exit(); } }",
    "begin { if (strncmp($a, $b, 3) < 0) { } }",
    "begin { if (has_key(@m, 1)) { print(@m[2]); } }",
    "begin { if (has_key(@m, 1)) { @m[1] = 2; } }",
    "begin { print(getopt(\"x\", 1)); }",
};
// clang-format on

std::string join(const std::vector<std::string> &codes)
{
    std::string out;
    for (const auto &c : codes)
        out += (out.empty() ? "" : ", ") + c;
    return out;
}

} // namespace

describe lints_test("lint", $ {
    for (const auto &c : cases) {
        const std::string title = c.code + " in: " + c.text;
        it(title.c_str(), _ {
            auto lint = lint_with(c.text, c.code);
            expect(lint.has_value()).to_be_true();
            if (!lint.has_value()) {
                expect(join(codes_of(c.text))).to_equal(c.code);
                return;
            }
            expect(lint->diagnostic.severity.has_value() &&
                   *lint->diagnostic.severity == c.severity)
                .to_be_true();
            expect(lint->diagnostic.source.value_or(""))
                .to_equal(std::string("bpfls"));
            if (c.fixed.has_value()) {
                expect(lint->fixes.empty()).to_be_false();
                if (!lint->fixes.empty())
                    expect(apply(c.text, lint->fixes.front()))
                        .to_equal(*c.fixed);
            } else {
                expect(lint->fixes.empty()).to_be_true();
            }
        });
    }

    it("flags every arrow in a chain", _ {
        auto codes = codes_of("begin { $x = $p->a->b; }");
        expect(join(codes)).to_equal(std::string("legacy-arrow, legacy-arrow"));
    });

    it("offers no did-you-mean for keys nothing like a known one", _ {
        auto lint = lint_with("config = { nonsense = 1 }\nbegin { }",
                              "unknown-config");
        expect(lint.has_value() && lint->fixes.empty()).to_be_true();
    });

    it("uses bpftrace's messages for config keys", _ {
        auto lint = lint_with("config = { nonsense = 1 }\nbegin { }",
                              "unknown-config");
        expect(lint.has_value()).to_be_true();
        if (lint.has_value())
            expect(std::get<std::string>(lint->diagnostic.message))
                .to_equal(
                    std::string("nonsense: not a known configuration option"));
    });

    it("tags deprecated constructs so editors can strike them through", _ {
        auto lint = lint_with("kfunc:f { }", "deprecated-probe-type");
        expect(lint.has_value() && lint->diagnostic.tags.has_value() &&
               lint->diagnostic.tags->size() == 1 &&
               lint->diagnostic.tags->front() == lsp::DiagnosticTag::Deprecated)
            .to_be_true();
    });

    for (const auto &text : clean) {
        const std::string title = "finds nothing in: " + text;
        it(title.c_str(), _ {
            expect(join(codes_of(text))).to_equal(std::string());
        });
    }
});

CPPSPEC_MAIN(lints_test);
