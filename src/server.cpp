#include "server.h"

#include <lsp/connection.h>

namespace bpfls {

Server::Server(lsp::io::Stream &stream) : endpoint_(stream)
{
    endpoint_.onInitialize(
        [](lsp::InitializeParams &&) -> lsp::InitializeResult {
        return {
            .capabilities = {},
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
