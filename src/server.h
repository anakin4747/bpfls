#pragma once

#include <lsp/io/stream.h>
#include <lsp/server_endpoint.h>

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
    lsp::ServerEndpoint endpoint_;
};

} // namespace bpfls
