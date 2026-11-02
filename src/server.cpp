#include "server.h"

#include <algorithm>
#include <string>
#include <string_view>

#include <lsp/connection.h>

#include "diagnostics.h"
#include "formatting.h"
#include "hover_docs.h"
#include "lints.h"

namespace bpfls {

Server::Server(lsp::io::Stream &stream) : endpoint_(stream)
{
    endpoint_.onInitialize(
        [](lsp::InitializeParams &&) -> lsp::InitializeResult {
        return {
            .capabilities = {
                .textDocumentSync = lsp::TextDocumentSyncKind::Full,
                .hoverProvider = true,
                .codeActionProvider =
                    lsp::CodeActionOptions{
                        .codeActionKinds = lsp::Array<lsp::CodeActionKindEnum>{
                            lsp::CodeActionKind::QuickFix },
                    },
                // Whole documents only: bpftrace's printer lays out complete
                // programs, so range and on-type formatting are left
                // unadvertised.
                .documentFormattingProvider = true,
            },
            .serverInfo = lsp::ServerInfo{ .name = "bpfls" },
        };
    });

    endpoint_.onShutdown([this]() -> lsp::ShutdownResult {
        shutdown_requested_ = true;
        return nullptr;
    });

    // Nothing to clean up, but the endpoint only ends its message loop on
    // `exit` when a handler is registered for it.
    endpoint_.onExit([] {});

    endpoint_.onTextDocumentDidOpen(
        [this](lsp::DidOpenTextDocumentParams &&params) {
        update(params.textDocument.uri,
               params.textDocument.version,
               std::move(params.textDocument.text));
    });

    // Only full-document sync is advertised, so the last change holds the
    // whole new text. Editors send this on every edit, not just on save, so
    // diagnostics follow the text as it is typed.
    endpoint_.onTextDocumentDidChange(
        [this](lsp::DidChangeTextDocumentParams &&params) {
        if (params.contentChanges.empty())
            return;
        auto *whole = std::get_if<lsp::TextDocumentContentChangeWholeDocument>(
            &params.contentChanges.back());
        if (whole == nullptr)
            return;
        update(params.textDocument.uri,
               params.textDocument.version,
               std::move(whole->text));
    });

    endpoint_.onTextDocumentDidClose(
        [this](lsp::DidCloseTextDocumentParams &&params) {
        documents_.erase(params.textDocument.uri.toString());
        // Clear the closed document's diagnostics in the editor.
        endpoint_.textDocumentPublishDiagnostics({
            .uri = params.textDocument.uri,
            .diagnostics = {},
        });
    });

    endpoint_.onTextDocumentHover(
        [this](lsp::HoverParams &&params) { return hover(params); });

    endpoint_.onTextDocumentFormatting(
        [this](lsp::DocumentFormattingParams &&params) {
        return formatting(params);
    });

    endpoint_.onTextDocumentCodeAction(
        [this](lsp::CodeActionParams &&params) { return code_action(params); });
}

void Server::update(const lsp::DocumentUri &uri, int version, std::string text)
{
    auto key = uri.toString();
    auto document = std::make_unique<Document>(key, std::move(text));
    endpoint_.textDocumentPublishDiagnostics({
        .uri = uri,
        .diagnostics = diagnostics(*document),
        .version = version,
    });
    documents_.insert_or_assign(key, std::move(document));
}

lsp::TextDocumentHoverResult Server::hover(const lsp::HoverParams &params)
{
    auto it = documents_.find(params.textDocument.uri.toString());
    if (it == documents_.end())
        return nullptr;

    std::string docs = hover_docs(it->second->node_at(params.position));
    if (docs.empty())
        return nullptr;

    return lsp::Hover{
        .contents =
            lsp::MarkupContent{
                .kind = lsp::MarkupKind::Markdown,
                .value = std::move(docs),
            },
    };
}

namespace {

// The position just past the last character of `text`. LSP counts
// characters in UTF-16 code units.
lsp::Position end_of(std::string_view text)
{
    lsp::Position end{ .line = 0, .character = 0 };
    for (unsigned char c : text) {
        if (c == '\n') {
            ++end.line;
            end.character = 0;
        } else if ((c & 0xC0) != 0x80) {
            // Each UTF-8 lead byte starts a code point; those beyond the BMP
            // (4-byte sequences) take two UTF-16 units.
            end.character += c >= 0xF0 ? 2 : 1;
        }
    }
    return end;
}

} // namespace

lsp::TextDocumentFormattingResult Server::formatting(
    const lsp::DocumentFormattingParams &params)
{
    auto it = documents_.find(params.textDocument.uri.toString());
    if (it == documents_.end())
        return nullptr;

    Document &document = *it->second;
    lsp::Array<lsp::TextEdit> edits;
    auto formatted = format(document);
    if (formatted.has_value() && *formatted != document.text()) {
        edits.push_back({
            .range = { .start = { .line = 0, .character = 0 },
                       .end = end_of(document.text()) },
            .newText = std::move(*formatted),
        });
    }
    return edits;
}

namespace {

bool before(const lsp::Position &a, const lsp::Position &b)
{
    return a.line < b.line || (a.line == b.line && a.character < b.character);
}

// Whether two ranges overlap or touch, so that a cursor at either end of a
// diagnostic still gets its fixes.
bool touches(const lsp::Range &a, const lsp::Range &b)
{
    return !before(a.end, b.start) && !before(b.end, a.start);
}

// Whether a client that asked only for `only` kinds wants quick fixes.
// Kinds are hierarchical: "" and "quickfix" both cover "quickfix".
bool wants_quick_fixes(const lsp::CodeActionContext &context)
{
    if (!context.only.has_value())
        return true;
    return std::ranges::any_of(*context.only, [](const auto &kind) {
        std::string value = static_cast<std::string>(kind);
        return value.empty() || value == "quickfix";
    });
}

} // namespace

lsp::TextDocumentCodeActionResult Server::code_action(
    const lsp::CodeActionParams &params)
{
    auto it = documents_.find(params.textDocument.uri.toString());
    if (it == documents_.end())
        return nullptr;

    lsp::Array<lsp::OneOf<lsp::Command, lsp::CodeAction>> actions;
    if (!wants_quick_fixes(params.context))
        return actions;

    for (auto &lint : lint(*it->second)) {
        if (!touches(lint.diagnostic.range, params.range))
            continue;
        // Only one fix is unambiguous enough to apply on its own.
        bool preferred = lint.fixes.size() == 1;
        for (auto &fix : lint.fixes) {
            lsp::Map<lsp::DocumentUri, lsp::Array<lsp::TextEdit>> changes;
            changes[params.textDocument.uri] = std::move(fix.edits);
            actions.push_back(lsp::CodeAction{
                .title = std::move(fix.title),
                .kind = lsp::CodeActionKind::QuickFix,
                .diagnostics = lsp::Array<lsp::Diagnostic>{ lint.diagnostic },
                .isPreferred = preferred,
                .edit = lsp::WorkspaceEdit{ .changes = std::move(changes) },
            });
        }
    }
    return actions;
}

int Server::run()
{
    try {
        endpoint_.runMessageLoop();
    } catch (const lsp::ConnectionError &) {
        // The stream ended before `exit`: the client went away.
        return 1;
    }
    // The endpoint enforces the lifecycle, so `exit` without a preceding
    // `shutdown` still ends the loop; the spec asks for exit code 1 then.
    return shutdown_requested_ ? 0 : 1;
}

} // namespace bpfls
