#include "server.h"

#include <string>
#include <string_view>

#include <lsp/connection.h>

#include "diagnostics.h"
#include "hover_docs.h"

namespace bpfls {

Server::Server(lsp::io::Stream &stream) : endpoint_(stream)
{
    endpoint_.onInitialize(
        [](lsp::InitializeParams &&) -> lsp::InitializeResult {
        return {
            .capabilities = {
                .textDocumentSync = lsp::TextDocumentSyncKind::Full,
                .hoverProvider = true,
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
