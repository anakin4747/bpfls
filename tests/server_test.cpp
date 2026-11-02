#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// cppspec defines macros such as `it`, which clash with identifiers in
// bpftrace's headers, so project headers must be included first.
#include "bpf_docs.h"
#include "server.h"
#include "support/memory_stream.h"

#include "cppspec.hpp"

using bpfls::test::frame;
using bpfls::test::messages;

namespace {

const std::string initialize = frame(
    R"({"jsonrpc":"2.0","id":1,"method":"initialize",)"
    R"("params":{"processId":null,"rootUri":null,"capabilities":{}}})");
const std::string initialized = frame(
    R"({"jsonrpc":"2.0","method":"initialized","params":{}})");
const std::string shutdown = frame(
    R"({"jsonrpc":"2.0","id":2,"method":"shutdown"})");
const std::string exit_ = frame(R"({"jsonrpc":"2.0","method":"exit"})");

// Quotes and escapes `text` as a JSON string.
std::string json_string(std::string_view text)
{
    std::string out = "\"";
    for (char c : text) {
        switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\n':
                out += "\\n";
                break;
            default:
                out += c;
        }
    }
    return out + "\"";
}

const std::string uri = "file:///t.bt";

std::string did_open(std::string_view text)
{
    return frame(
        R"({"jsonrpc":"2.0","method":"textDocument/didOpen","params":{)"
        R"("textDocument":{"uri":")" +
        uri + R"(","languageId":"bpftrace","version":1,"text":)" +
        json_string(text) + "}}}");
}

std::string did_change(std::string_view text, int version = 2)
{
    return frame(
        R"({"jsonrpc":"2.0","method":"textDocument/didChange","params":{)"
        R"("textDocument":{"uri":")" +
        uri + R"(","version":)" + std::to_string(version) +
        R"(},"contentChanges":[{"text":)" + json_string(text) + "}]}}");
}

std::string did_close()
{
    return frame(
        R"({"jsonrpc":"2.0","method":"textDocument/didClose","params":{)"
        R"("textDocument":{"uri":")" +
        uri + R"("}}})");
}

// Runs a session of `messages_in` after initialization and returns the
// params of every textDocument/publishDiagnostics notification, in order.
std::vector<lsp::json::Value> published(const std::string &messages_in)
{
    bpfls::test::MemoryStream stream(initialize + initialized + messages_in);
    bpfls::Server(stream).run();

    std::vector<lsp::json::Value> result;
    for (auto &message : messages(stream.output())) {
        auto &object = message.object();
        if (object.contains("method") &&
            object.get("method").string() == "textDocument/publishDiagnostics")
            result.push_back(object.get("params"));
    }
    return result;
}

// The diagnostics array of published params.
const lsp::json::Array &diagnostics_of(const lsp::json::Value &params)
{
    return params.object().get("diagnostics").array();
}

// A hover request with id 2.
std::string hover(unsigned line,
                  unsigned character,
                  std::string_view hover_uri = uri)
{
    return frame(
        R"({"jsonrpc":"2.0","id":2,"method":"textDocument/hover","params":{)"
        R"("textDocument":{"uri":")" +
        std::string(hover_uri) + R"("},"position":{"line":)" +
        std::to_string(line) + R"(,"character":)" + std::to_string(character) +
        "}}}");
}

// Runs a session of `messages_in` after initialization and returns the
// hover response's Markdown, or nullopt if the result was null. Anything
// else (no response, an error, other content) fails the calling test.
std::optional<std::string> hover_session(const std::string &messages_in)
{
    bpfls::test::MemoryStream stream(initialize + initialized + messages_in);
    bpfls::Server(stream).run();

    for (auto &message : messages(stream.output())) {
        auto &object = message.object();
        if (!object.contains("id") || object.get("id").number() != 2.0)
            continue;
        if (!object.contains("result"))
            return "<error response>";
        auto &result = object.get("result");
        if (result.isNull())
            return std::nullopt;
        auto &contents = result.object().get("contents").object();
        if (contents.get("kind").string() != "markdown")
            return "<not markdown>";
        return contents.get("value").string();
    }
    return "<no hover response>";
}

// Hovers `line`:`character` in a document containing `text`.
std::optional<std::string> hover_at(std::string_view text,
                                    unsigned line,
                                    unsigned character)
{
    return hover_session(did_open(text) + hover(line, character));
}

// The lines of bpftrace/docs/<file> from `start` up to, but not including,
// `end`, read from disk now. Deliberately independent of bpf_docs() so the
// hover text is checked against the real docs file.
std::string docs_section(const std::string &file,
                         const std::string &start,
                         const std::string &end)
{
    auto path = std::filesystem::path(__FILE__).parent_path() /
                "../bpftrace/docs" / file;
    std::ifstream in(path);
    std::string line, section;
    bool inside = false;
    while (std::getline(in, line)) {
        if (line == start)
            inside = true;
        else if (line == end && inside)
            break;
        if (inside)
            section += line + "\n";
    }
    return section;
}

// A section of language.md, read from disk by the test.
std::string language_docs(const std::string &start, const std::string &end)
{
    return docs_section("language.md", start, end);
}

const std::string begin_end_docs = language_docs("### begin/end", "### test");

// A section of stdlib.md, read from disk by the test: its `### name` heading
// up to the next `##` or `###` heading.
struct StdlibSection {
    std::string name;
    std::string text;
};

// Every section of stdlib.md, read from disk by the test, so a section added
// upstream fails a test until hover supports it.
std::vector<StdlibSection> read_stdlib_sections()
{
    auto path = std::filesystem::path(__FILE__).parent_path() /
                "../bpftrace/docs/stdlib.md";
    std::ifstream in(path);
    std::vector<StdlibSection> sections;
    std::string line;
    bool inside = false;
    while (std::getline(in, line)) {
        if (line.starts_with("## ") || line.starts_with("### "))
            inside = false;
        if (line.starts_with("### ")) {
            sections.push_back({ line.substr(4), "" });
            inside = true;
        }
        if (inside)
            sections.back().text += line + "\n";
    }
    return sections;
}

const std::vector<StdlibSection> stdlib_sections = read_stdlib_sections();

// Names documented in both language.md and stdlib.md hover with both
// sections, each labelled with the file it comes from.
std::string joined_docs(const std::string &language, const std::string &stdlib)
{
    return "*From language.md*\n\n" + language +
           "\n---\n\n*From stdlib.md*\n\n" + stdlib;
}

const std::string type_context_docs = language_docs("### Type Context Builtins",
                                                    "### Pointers");

// The hover expected for stdlib section `s`.
std::string expected_stdlib_hover(const StdlibSection &s)
{
    if (s.name == "sizeof" || s.name == "offsetof" || s.name == "typeof")
        return joined_docs(type_context_docs, s.text);
    return s.text;
}

// A call to stdlib function `name` with arguments it needs to parse.
std::string stdlib_call(const std::string &name)
{
    if (name == "sizeof")
        return "sizeof(int)";
    if (name == "offsetof")
        return "offsetof(struct f, x)";
    return name + "()";
}

// A hover the server must give: hovering `line`:`character` in `text`
// shows the language.md section from `start` up to `end`.
struct HoverCase {
    std::string text;
    unsigned line;
    unsigned character;
    std::string start;
    std::string end;
};

// clang-format off
const std::vector<HoverCase> probe_cases = {
    { "kprobe:f { }",            0, 0, "### kprobe and kretprobe", "### profile" },
    { "k:f { }",                 0, 0, "### kprobe and kretprobe", "### profile" },
    { "kretprobe:f { }",         0, 0, "### kprobe and kretprobe", "### profile" },
    { "kr:f { }",                0, 0, "### kprobe and kretprobe", "### profile" },
    { "uprobe:/b:f { }",         0, 0, "### uprobe, uretprobe", "### usdt" },
    { "u:/b:f { }",              0, 0, "### uprobe, uretprobe", "### usdt" },
    { "uretprobe:/b:f { }",      0, 0, "### uprobe, uretprobe", "### usdt" },
    { "ur:/b:f { }",             0, 0, "### uprobe, uretprobe", "### usdt" },
    { "usdt:/b:p:f { }",         0, 0, "### usdt", "### watchpoint" },
    { "U:/b:p:f { }",            0, 0, "### usdt", "### watchpoint" },
    { "tracepoint:a:b { }",      0, 0, "### tracepoint", "### uprobe, uretprobe" },
    { "t:a:b { }",               0, 0, "### tracepoint", "### uprobe, uretprobe" },
    { "rawtracepoint:x { }",     0, 0, "### rawtracepoint", "### software" },
    { "rt:x { }",                0, 0, "### rawtracepoint", "### software" },
    { "profile:hz:99 { }",       0, 0, "### profile", "### rawtracepoint" },
    { "p:hz:99 { }",             0, 0, "### profile", "### rawtracepoint" },
    { "interval:s:1 { }",        0, 0, "### interval", "### iterator" },
    { "i:s:1 { }",               0, 0, "### interval", "### iterator" },
    { "software:faults:1 { }",   0, 0, "### software", "### tracepoint" },
    { "s:faults:1 { }",          0, 0, "### software", "### tracepoint" },
    { "hardware:cycles:1 { }",   0, 0, "### hardware", "### interval" },
    { "h:cycles:1 { }",          0, 0, "### hardware", "### interval" },
    { "watchpoint:0x1:8:rw { }", 0, 0, "### watchpoint", "## Types" },
    { "w:0x1:8:rw { }",          0, 0, "### watchpoint", "## Types" },
    { "fentry:x { }",            0, 0, "### fentry and fexit", "### kprobe and kretprobe" },
    { "f:x { }",                 0, 0, "### fentry and fexit", "### kprobe and kretprobe" },
    { "fexit:x { }",             0, 0, "### fentry and fexit", "### kprobe and kretprobe" },
    { "fr:x { }",                0, 0, "### fentry and fexit", "### kprobe and kretprobe" },
    { "iter:task { }",           0, 0, "### iterator", "### fentry and fexit" },
    { "it:task { }",             0, 0, "### iterator", "### fentry and fexit" },
    { "self:signal:SIGUSR1 { }", 0, 0, "### self", "### hardware" },
    { "test:a { }",              0, 0, "### test", "### bench" },
    { "bench:a { }",             0, 0, "### bench", "### self" },
};

// Each config variable's section runs up to the next one's.
const std::vector<std::string> config_variables = {
    "cache_user_symbols", "cpp_demangle", "lazy_symbolication", "license",
    "log_size", "max_bpf_progs", "max_cat_bytes", "max_map_keys",
    "max_probes", "max_strlen", "missing_probes", "on_stack_limit",
    "perf_rb_pages", "show_debug_info", "stack_mode", "str_trunc_trailer",
    "print_maps_on_exit",
};

// Columns of `begin { ... }` bodies below: `begin { ` is 8 characters, so
// the body starts at column 8 and `$a = ` puts an expression at column 13.
const std::vector<HoverCase> construct_cases = {
    { "config = { max_strlen = 1 }\nbegin { }", 0, 0, "## Config Block", "## Config Variables" },
    { "config = { BPFTRACE_MAX_STRLEN = 1 }\nbegin { }", 0, 12, "### max_strlen", "### missing_probes" },
    { "config = { unstable_tseries = enable }\nbegin { }", 0, 12, "### unstable features", "## Data Types" },
    { "begin { $a = args.x; }",            0, 13, "### args", "## Arrays" },
    { "begin { $a = arg0; }",              0, 13, "### argN", "### args" },
    { "begin { $a = arg12; }",             0, 13, "### argN", "### args" },
    { "begin { $a = $1; }",                0, 13, "### Positional Parameters", "## Comments" },
    { "begin { $a = $#; }",                0, 13, "### Positional Parameters", "## Comments" },
    { "begin { for $i : 0..3 { } }",       0, 8,  "### For", "### While" },
    { "begin { for $kv : @m { } }",        0, 8,  "### For", "### While" },
    { "begin { while (1) { } }",           0, 8,  "### While", "### Unroll" },
    { "begin { unroll(2) { } }",           0, 8,  "### Unroll", "## Macros" },
    { "begin { if (1) { } }",              0, 8,  "## Conditionals", "## Config Block" },
    { "begin { $a = 1 ? 2 : 3; }",         0, 15, "## Conditionals", "## Config Block" },
    { "import \"foo\";\nbegin { }",       0, 0,  "## Imports", "## Kernel Functions" },
    { "begin { import \"foo\"; }",        0, 8,  "## Imports", "## Kernel Functions" },
    { "macro f(x) { x }\nbegin { }",       0, 0,  "## Macros", "## Operators and Expressions" },
    { "let @m = hash(10);\nbegin { }",     0, 0,  "### Map Declarations", "### Maps without Explicit Keys" },
    { "begin { $a = (1, 2); }",            0, 13, "### Tuples", "### Records" },
    { "begin { $a = $t.0; }",              0, 15, "### Tuples", "### Records" },
    { "begin { $a = (x=1); }",             0, 13, "### Records", "### Type Conversion" },
    { "begin { $a = (uint8)1; }",          0, 13, "### Type Conversion", "## Variables and Maps" },
    { "begin { $a++; }",                   0, 10, "### Increment and Decrement Operators", "### Block Expressions" },
    { "begin { --$a; }",                   0, 8,  "### Increment and Decrement Operators", "### Block Expressions" },
    // sizeof, offsetof and typeof are also in stdlib.md; see below.
    { "begin { $a = typeinfo(1); }",       0, 13, "### Type Context Builtins", "### Pointers" },
};
// clang-format on

// Describes a case for its test name.
std::string describe_case(const HoverCase &c)
{
    return "shows \"" + c.start + "\" on " + std::to_string(c.line) + ":" +
           std::to_string(c.character) + " of `" + c.text + "`";
}

// The capabilities the server answers initialize with.
lsp::json::Object capabilities()
{
    bpfls::test::MemoryStream stream(initialize);
    bpfls::Server(stream).run();
    auto out = messages(stream.output());
    if (out.empty())
        return {};
    return out.at(0)
        .object()
        .get("result")
        .object()
        .get("capabilities")
        .object();
}

// A request with id 2 for `method` on the test document, with `extra`
// params appended after the text document.
std::string formatting_request(std::string_view method,
                               std::string_view extra = "")
{
    return frame(R"({"jsonrpc":"2.0","id":2,"method":")" + std::string(method) +
                 R"(","params":{"textDocument":{"uri":")" + uri +
                 R"("},"options":{"tabSize":4,"insertSpaces":true})" +
                 std::string(extra) + "}}");
}

std::string formatting()
{
    return formatting_request("textDocument/formatting");
}

// Runs a session of `messages_in` after initialization and returns the whole
// response to the request with id 2, or null if there is none.
lsp::json::Value response_2(const std::string &messages_in)
{
    bpfls::test::MemoryStream stream(initialize + initialized + messages_in);
    bpfls::Server(stream).run();
    for (auto &message : messages(stream.output())) {
        auto &object = message.object();
        if (object.contains("id") && object.get("id").isNumber() &&
            object.get("id").number() == 2.0)
            return message;
    }
    return nullptr;
}

// The result of formatting `text` after opening it.
lsp::json::Value format_result(std::string_view text)
{
    auto response = response_2(did_open(text) + formatting());
    if (!response.isObject() || !response.object().contains("result"))
        return std::string("<no result>");
    return response.object().get("result");
}

// Only compiles if the docs are extracted at build time.
static_assert(bpfls::bpf_docs("language.md", "### begin/end", "### test")
                  .starts_with("### begin/end"));

} // namespace

// A code action request with id 2 for `start_char`..`end_char` on `line`,
// optionally limited to the kinds in `only` (a JSON array).
std::string code_action(unsigned line,
                        unsigned start_char,
                        unsigned end_char,
                        const std::string &only = "")
{
    auto pos = [&](unsigned c) {
        return R"({"line":)" + std::to_string(line) + R"(,"character":)" +
               std::to_string(c) + "}";
    };
    return frame(
        R"({"jsonrpc":"2.0","id":2,"method":"textDocument/codeAction",)"
        R"("params":{"textDocument":{"uri":")" +
        uri + R"("},"range":{"start":)" + pos(start_char) + R"(,"end":)" +
        pos(end_char) + R"(},"context":{"diagnostics":[])" +
        (only.empty() ? "" : R"(,"only":)" + only) + "}}}");
}

describe server_test("Server", $ {
    it("answers initialize with its name", _ {
        bpfls::test::MemoryStream stream(initialize);
        bpfls::Server(stream).run();

        auto out = messages(stream.output());
        expect(out.size()).to_equal(1UL);
        if (out.size() == 1UL) {
            auto &response = out.at(0).object();
            expect(response.get("id").number()).to_equal(1.0);
            auto &info =
                response.get("result").object().get("serverInfo").object();
            expect(info.get("name").string()).to_equal(std::string("bpfls"));
        }
    });

    it("answers shutdown with a null result", _ {
        bpfls::test::MemoryStream stream(initialize + initialized + shutdown +
                                         exit_);
        bpfls::Server(stream).run();

        auto out = messages(stream.output());
        expect(out.size()).to_equal(2UL);
        if (out.size() == 2UL) {
            auto &response = out.at(1).object();
            expect(response.get("id").number()).to_equal(2.0);
            expect(response.get("result").isNull()).to_be_true();
        }
    });

    it("exits with 0 when exit follows shutdown", _ {
        bpfls::test::MemoryStream stream(initialize + initialized + shutdown +
                                         exit_);
        expect(bpfls::Server(stream).run()).to_equal(0);
    });

    it("exits with 1 when exit arrives without shutdown", _ {
        bpfls::test::MemoryStream stream(initialize + initialized + exit_);
        expect(bpfls::Server(stream).run()).to_equal(1);
    });

    it("exits with 1 when the input ends without exit", _ {
        bpfls::test::MemoryStream stream("");
        expect(bpfls::Server(stream).run()).to_equal(1);
    });

    it("rejects requests sent before initialize", _ {
        bpfls::test::MemoryStream stream(shutdown);
        bpfls::Server(stream).run();

        auto out = messages(stream.output());
        expect(out.size()).to_equal(1UL);
        if (out.size() == 1UL) {
            auto &error = out.at(0).object().get("error").object();
            expect(error.get("code").number()).to_equal(-32002.0);
        }
    });

    context("hover", _ {
        it("has the docs to show", _ {
            // Guards the tests below against comparing two empty strings.
            expect(begin_end_docs.starts_with("### begin/end\n")).to_be_true();
        });

        it("shows the begin/end docs on the first character of begin", _ {
            expect(hover_at("begin { }", 0, 0) == begin_end_docs).to_be_true();
        });

        it("shows the begin/end docs on the last character of begin", _ {
            expect(hover_at("begin { }", 0, 4) == begin_end_docs).to_be_true();
        });

        it("shows the begin/end docs on the first character of end", _ {
            expect(hover_at("end { }", 0, 0) == begin_end_docs).to_be_true();
        });

        it("shows the begin/end docs on the last character of end", _ {
            expect(hover_at("end { }", 0, 2) == begin_end_docs).to_be_true();
        });

        it("shows the docs for each of several attach points", _ {
            expect(hover_at("begin, end { }", 0, 2) == begin_end_docs)
                .to_be_true();
            expect(hover_at("begin, end { }", 0, 8) == begin_end_docs)
                .to_be_true();
        });

        it("shows nothing on the probe's braces", _ {
            expect(hover_at("begin { }", 0, 6) == std::nullopt).to_be_true();
            expect(hover_at("begin { }", 0, 8) == std::nullopt).to_be_true();
        });

        it("shows nothing inside the probe's body", _ {
            // On the `1`; `print` itself has stdlib docs.
            expect(hover_at("begin { print(1); }", 0, 14) == std::nullopt)
                .to_be_true();
        });

        it("shows the begin/end docs on uppercase BEGIN and END", _ {
            // bpftrace matches probe providers case-insensitively.
            expect(hover_at("BEGIN { }", 0, 2) == begin_end_docs).to_be_true();
            expect(hover_at("END { }", 0, 1) == begin_end_docs).to_be_true();
        });

        it("goes by the provider, not by begin appearing later on", _ {
            expect(hover_at("uprobe:/bin/sh:begin { }", 0, 16) ==
                   language_docs("### uprobe, uretprobe", "### usdt"))
                .to_be_true();
        });

        it("shows nothing on a wildcard provider", _ {
            expect(hover_at("k*:foo { }", 0, 0) == std::nullopt).to_be_true();
        });

        it("shows nothing between probes", _ {
            expect(hover_at("begin { }\n\nend { }\n", 1, 0) == std::nullopt)
                .to_be_true();
        });

        it("shows nothing in a document that was never opened", _ {
            expect(hover_session(hover(0, 0, "file:///other.bt")) ==
                   std::nullopt)
                .to_be_true();
        });

        it("uses the latest text after a change", _ {
            expect(hover_session(did_open("begin { }") + did_change("     ") +
                                 hover(0, 2)) == std::nullopt)
                .to_be_true();
        });

        context("probes", _ {
            for (const auto &c : probe_cases) {
                const std::string name = describe_case(c);
                it(name.c_str(), _ {
                    expect(hover_at(c.text, c.line, c.character) ==
                           language_docs(c.start, c.end))
                        .to_be_true();
                });
            }
        });

        context("config variables", _ {
            for (std::size_t i = 0; i < config_variables.size(); ++i) {
                const std::string name = config_variables[i];
                const std::string next = i + 1 < config_variables.size()
                                             ? "### " + config_variables[i + 1]
                                             : "### unstable features";
                const std::string title = "shows \"### " + name +
                                          "\" on its name";
                it(title.c_str(), _ {
                    // `config = { ` is 11 characters.
                    expect(hover_at("config = { " + name + " = 1 }\nbegin { }",
                                    0,
                                    11) == language_docs("### " + name, next))
                        .to_be_true();
                });
            }
        });

        context("language constructs", _ {
            for (const auto &c : construct_cases) {
                const std::string name = describe_case(c);
                it(name.c_str(), _ {
                    expect(hover_at(c.text, c.line, c.character) ==
                           language_docs(c.start, c.end))
                        .to_be_true();
                });
            }

            it("shows nothing on the type inside a cast", _ {
                expect(hover_at("begin { $a = (uint8)1; }", 0, 15) ==
                       std::nullopt)
                    .to_be_true();
            });

            it("shows nothing on a function it has no docs for", _ {
                expect(hover_at("begin { my_func(1); }", 0, 8) == std::nullopt)
                    .to_be_true();
            });
        });

        context("standard library", _ {
            it("has the stdlib.md sections to check against", _ {
                expect(stdlib_sections.size() > 100).to_be_true();
            });

            // `begin { $a = ` is 13 characters.
            for (const auto &section : stdlib_sections) {
                const std::string call = stdlib_call(section.name);
                const std::string call_title = "shows \"### " + section.name +
                                               "\" on `" + call + "`";
                it(call_title.c_str(), _ {
                    expect(hover_at("begin { $a = " + call + "; }", 0, 13) ==
                           expected_stdlib_hover(section))
                        .to_be_true();
                });

                // Written without parentheses, e.g. `pid` or `comm`.
                if (section.name == "sizeof" || section.name == "offsetof")
                    continue;
                const std::string bare_title = "shows \"### " + section.name +
                                               "\" on bare `" + section.name +
                                               "`";
                it(bare_title.c_str(), _ {
                    expect(hover_at("begin { $a = " + section.name + "; }",
                                    0,
                                    13) == expected_stdlib_hover(section))
                        .to_be_true();
                });
            }

            it("shows nothing on internal builtins", _ {
                expect(hover_at("begin { $a = __builtin_comm; }", 0, 13) ==
                       std::nullopt)
                    .to_be_true();
            });
        });

        it("still works in a document with syntax errors elsewhere", _ {
            expect(hover_at("begin { }\nend { $x = ; }\n", 0, 1) ==
                   begin_end_docs)
                .to_be_true();
        });
    });

    context("diagnostics", _ {
        it("publishes a parser error when a document is opened", _ {
            auto out = published(did_open("begin { $x = ; }"));
            expect(out.size()).to_equal(1UL);
            if (out.size() != 1UL)
                return;
            auto &params = out.at(0).object();
            expect(params.get("uri").string()).to_equal(uri);
            expect(params.get("version").number()).to_equal(1.0);

            auto &diagnostics = diagnostics_of(out.at(0));
            expect(diagnostics.size()).to_equal(1UL);
            if (diagnostics.size() != 1UL)
                return;
            auto &d = diagnostics.at(0).object();
            expect(d.get("message").string())
                .to_equal(std::string("syntax: expected expression"));
            expect(d.get("severity").number()).to_equal(1.0); // Error
            expect(d.get("source").string()).to_equal(std::string("bpftrace"));

            // The error is on the `;` at 0-based column 13.
            auto &range = d.get("range").object();
            auto &start = range.get("start").object();
            auto &end = range.get("end").object();
            expect(start.get("line").number()).to_equal(0.0);
            expect(start.get("character").number()).to_equal(13.0);
            expect(end.get("line").number()).to_equal(0.0);
            expect(end.get("character").number()).to_equal(14.0);
        });

        it("publishes every error, not just the first", _ {
            auto out = published(did_open("begin { $x = ; }\n"
                                          "end { print( }\n"));
            expect(out.size()).to_equal(1UL);
            if (out.size() != 1UL)
                return;
            auto &diagnostics = diagnostics_of(out.at(0));
            expect(diagnostics.size()).to_equal(2UL);
            if (diagnostics.size() != 2UL)
                return;
            auto line = [&](std::size_t i) {
                return diagnostics.at(i)
                    .object()
                    .get("range")
                    .object()
                    .get("start")
                    .object()
                    .get("line")
                    .number();
            };
            expect(line(0)).to_equal(0.0);
            expect(line(1)).to_equal(1.0);
        });

        it("publishes no diagnostics for a valid document", _ {
            auto out = published(did_open("begin { print(1); }"));
            expect(out.size()).to_equal(1UL);
            if (out.size() == 1UL)
                expect(diagnostics_of(out.at(0)).empty()).to_be_true();
        });

        it("clears the diagnostics when a change fixes the error", _ {
            auto out = published(did_open("begin { $x = ; }") +
                                 did_change("begin { $x = 1; }"));
            expect(out.size()).to_equal(2UL);
            if (out.size() != 2UL)
                return;
            expect(diagnostics_of(out.at(0)).size()).to_equal(1UL);
            expect(diagnostics_of(out.at(1)).empty()).to_be_true();
            expect(out.at(1).object().get("version").number()).to_equal(2.0);
        });

        it("follows every change, ending with the latest text", _ {
            auto out = published(did_open("begin { }") +
                                 did_change("begin { $x = ; }", 2) +
                                 did_change("begin { }", 3) +
                                 did_change("begin { }\nend { $y = ; }", 4));
            expect(out.size()).to_equal(4UL);
            if (out.size() != 4UL)
                return;
            expect(diagnostics_of(out.at(0)).size()).to_equal(0UL);
            expect(diagnostics_of(out.at(1)).size()).to_equal(1UL);
            expect(diagnostics_of(out.at(2)).size()).to_equal(0UL);
            auto &last = diagnostics_of(out.at(3));
            expect(last.size()).to_equal(1UL);
            if (last.size() == 1UL) {
                expect(last.at(0)
                           .object()
                           .get("range")
                           .object()
                           .get("start")
                           .object()
                           .get("line")
                           .number())
                    .to_equal(1.0);
            }
            expect(out.at(3).object().get("version").number()).to_equal(4.0);
        });

        it("clears the diagnostics when the document is closed", _ {
            auto out = published(did_open("begin { $x = ; }") + did_close());
            expect(out.size()).to_equal(2UL);
            if (out.size() != 2UL)
                return;
            expect(out.at(1).object().get("uri").string()).to_equal(uri);
            expect(diagnostics_of(out.at(1)).empty()).to_be_true();
        });
    });
    context("code actions", _ {
        it("advertises quick fixes", _ {
            auto caps = capabilities();
            expect(caps.contains("codeActionProvider")).to_be_true();
            if (caps.contains("codeActionProvider")) {
                auto &kinds = caps.get("codeActionProvider")
                                  .object()
                                  .get("codeActionKinds")
                                  .array();
                expect(kinds.size() == 1 && kinds.at(0).string() == "quickfix")
                    .to_be_true();
            }
        });

        it("offers the fix for a lint under the cursor", _ {
            auto response = response_2(did_open("kfunc:f { }") +
                                       code_action(0, 2, 2));
            auto &result = response.object().get("result");
            expect(result.isArray() && result.array().size() == 1).to_be_true();
            if (!result.isArray() || result.array().size() != 1)
                return;
            auto &action = result.array().at(0).object();
            expect(action.get("title").string())
                .to_equal(std::string("Replace with fentry"));
            expect(action.get("kind").string())
                .to_equal(std::string("quickfix"));
            expect(action.get("isPreferred").boolean()).to_be_true();
            expect(action.get("diagnostics").array().size()).to_equal(1UL);
            auto &edits = action.get("edit")
                              .object()
                              .get("changes")
                              .object()
                              .get(uri)
                              .array();
            expect(edits.size()).to_equal(1UL);
            if (edits.size() == 1UL)
                expect(edits.at(0).object().get("newText").string())
                    .to_equal(std::string("fentry"));
        });

        it("offers nothing away from any lint", _ {
            auto response = response_2(did_open("kfunc:f { }\nbegin { }") +
                                       code_action(1, 0, 3));
            auto &result = response.object().get("result");
            expect(result.isArray() && result.array().empty()).to_be_true();
        });

        it("offers nothing to a client that only wants refactorings", _ {
            auto response = response_2(did_open("kfunc:f { }") +
                                       code_action(0, 2, 2, R"(["refactor"])"));
            auto &result = response.object().get("result");
            expect(result.isArray() && result.array().empty()).to_be_true();
        });

        it("offers every did-you-mean fix, none preferred", _ {
            auto response = response_2(
                did_open("config = { max_probe = 1 }\nbegin { }") +
                code_action(0, 12, 12));
            auto &result = response.object().get("result");
            expect(result.isArray() && result.array().size() >= 1).to_be_true();
            if (!result.isArray())
                return;
            for (const auto &action : result.array())
                expect(action.object().get("isPreferred").boolean())
                    .to_equal(result.array().size() == 1);
        });

        it("returns null for a document that was never opened", _ {
            auto response = response_2(code_action(0, 0, 0));
            expect(response.object().get("result").isNull()).to_be_true();
        });
    });

    context("formatting", _ {
        it("advertises formatting whole documents", _ {
            auto caps = capabilities();
            expect(caps.contains("documentFormattingProvider")).to_be_true();
            if (caps.contains("documentFormattingProvider")) {
                auto &provider = caps.get("documentFormattingProvider");
                expect(provider.isBoolean() && provider.boolean()).to_be_true();
            }
        });

        it("does not advertise formatting ranges", _ {
            auto caps = capabilities();
            expect(caps.contains("documentRangeFormattingProvider"))
                .to_be_false();
        });

        it("does not advertise formatting as you type", _ {
            auto caps = capabilities();
            expect(caps.contains("documentOnTypeFormattingProvider"))
                .to_be_false();
        });

        it("rejects range formatting requests", _ {
            auto response = response_2(
                did_open("begin{print(1)}") +
                formatting_request("textDocument/rangeFormatting",
                                   R"(,"range":{"start":{"line":0,)"
                                   R"("character":0},"end":{"line":0,)"
                                   R"("character":5}})"));
            expect(response.isObject() && response.object().contains("error"))
                .to_be_true();
        });

        it("replaces the whole document with bpftrace's layout", _ {
            auto result = format_result("begin{print(1)}\n\nend{exit()}\n");
            expect(result.isArray() && result.array().size() == 1).to_be_true();
            if (result.isArray() && result.array().size() == 1) {
                auto &edit = result.array().at(0).object();
                auto &range = edit.get("range").object();
                auto &start = range.get("start").object();
                auto &end = range.get("end").object();
                expect(start.get("line").number()).to_equal(0.0);
                expect(start.get("character").number()).to_equal(0.0);
                expect(end.get("line").number()).to_equal(3.0);
                expect(end.get("character").number()).to_equal(0.0);
                expect(edit.get("newText").string())
                    .to_equal(std::string("begin\n{\n  print(1);\n}\n\n"
                                          "end\n{\n  exit();\n}\n"));
            }
        });

        it("ends the edit at the end of an unterminated last line", _ {
            auto result = format_result(
                "begin{print(1)}\n// h\u00e9\U0001F600");
            expect(result.isArray() && result.array().size() == 1).to_be_true();
            if (result.isArray() && result.array().size() == 1) {
                auto &end = result.array()
                                .at(0)
                                .object()
                                .get("range")
                                .object()
                                .get("end")
                                .object();
                expect(end.get("line").number()).to_equal(1.0);
                // "// h", one UTF-16 unit for é and two for the emoji.
                expect(end.get("character").number()).to_equal(7.0);
            }
        });

        it("makes no edits to a formatted document", _ {
            auto result = format_result("begin\n{\n  print(1);\n}\n");
            expect(result.isArray() && result.array().empty()).to_be_true();
        });

        it("makes no edits to a document with syntax errors", _ {
            auto result = format_result("begin { $x = ; }\n");
            expect(result.isArray() && result.array().empty()).to_be_true();
        });

        it("formats the latest text after a change", _ {
            auto response = response_2(did_open("begin { }") +
                                       did_change("end{exit()}") +
                                       formatting());
            lsp::json::Value result = nullptr;
            if (response.isObject() && response.object().contains("result"))
                result = response.object().get("result");
            expect(result.isArray() && result.array().size() == 1).to_be_true();
            if (result.isArray() && result.array().size() == 1) {
                expect(result.array().at(0).object().get("newText").string())
                    .to_equal(std::string("end\n{\n  exit();\n}\n"));
            }
        });

        it("returns null for a document that was never opened", _ {
            auto response = response_2(formatting());
            expect(response.isObject() &&
                   response.object().contains("result") &&
                   response.object().get("result").isNull())
                .to_be_true();
        });
    });
});

CPPSPEC_MAIN(server_test);
