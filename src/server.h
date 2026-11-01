#pragma once

#include <map>
#include <memory>
#include <string>

#include <lsp/io/stream.h>
#include <lsp/server_endpoint.h>

#include "document.h"

namespace bpfls {

// The bpftrace language server: speaks LSP over the given stream.
class Server {
public:
    explicit Server(lsp::io::Stream &stream);

    // Processes messages until the client sends `exit` or the stream ends.
    // Returns the process exit code the LSP specification asks for: 0 if
    // `shutdown` was received before `exit`, 1 otherwise.
    int run();

private:
    lsp::TextDocumentHoverResult hover(const lsp::HoverParams &params);

    lsp::ServerEndpoint endpoint_;
    bool shutdown_requested_ = false;
    // Open documents by URI. Held by pointer because a Document's AST refers
    // back into it, so it must not move.
    std::map<std::string, std::unique_ptr<Document>> documents_;
};

} // namespace bpfls
