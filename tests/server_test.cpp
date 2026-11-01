#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>

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

std::string did_change(std::string_view text)
{
    return frame(
        R"({"jsonrpc":"2.0","method":"textDocument/didChange","params":{)"
        R"("textDocument":{"uri":")" +
        uri + R"(","version":2},"contentChanges":[{"text":)" +
        json_string(text) + "}]}}");
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

const std::string begin_end_docs = docs_section("language.md",
                                                "### begin/end",
                                                "### test");

// Only compiles if the docs are extracted at build time.
static_assert(bpfls::bpf_docs("language.md", "### begin/end", "### test")
                  .starts_with("### begin/end"));

} // namespace

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
            expect(hover_at("begin { print(1); }", 0, 9) == std::nullopt)
                .to_be_true();
        });

        it("shows nothing on uppercase BEGIN and END", _ {
            expect(hover_at("BEGIN { }", 0, 2) == std::nullopt).to_be_true();
            expect(hover_at("END { }", 0, 1) == std::nullopt).to_be_true();
        });

        it("shows nothing on other probes", _ {
            expect(hover_at("kprobe:foo { }", 0, 2) == std::nullopt)
                .to_be_true();
            expect(hover_at("uprobe:/bin/sh:begin { }", 0, 16) == std::nullopt)
                .to_be_true();
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
            expect(hover_session(did_open("begin { }") +
                                 did_change("kprobe:foo { }") + hover(0, 2)) ==
                   std::nullopt)
                .to_be_true();
        });
    });
});

CPPSPEC_MAIN(server_test);
