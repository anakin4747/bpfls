// Starts the real bpfls binary and talks to it the way an editor would: over
// its stdin and stdout, using lsp-framework's client endpoint.

#include <string>
#include <thread>

#include <lsp/client_endpoint.h>
#include <lsp/connection.h>
#include <lsp/process.h>

#include "cppspec.hpp"

namespace {

// Runs the client's message loop on a background thread, so responses can be
// awaited from the test thread, and joins it on destruction. The loop ends
// once the server closes its stdout by exiting.
class ClientLoop {
public:
    explicit ClientLoop(lsp::ClientEndpoint &client)
        : thread_([&client] {
              try {
                  client.runMessageLoop();
              } catch (const lsp::ConnectionError &) {
                  // The server went away while the client still expected
                  // messages; the test's own expectations report why.
              }
          })
    {
    }

    ~ClientLoop()
    {
        thread_.join();
    }

private:
    std::thread thread_;
};

lsp::InitializeParams initialize_params()
{
    lsp::InitializeParams params;
    params.processId = lsp::Process::currentProcessId();
    return params;
}

} // namespace

describe lifecycle_test("bpfls", $ {
    it("starts, identifies itself, shuts down and exits with 0", _ {
        auto process = lsp::Process::start(BPFLS_EXECUTABLE);
        lsp::ClientEndpoint client(process.stdIO());
        int exit_code = -1;
        {
            ClientLoop loop(client);

            auto result = client.initialize(initialize_params()).get();
            expect(result.serverInfo.has_value()).to_be_true();
            if (result.serverInfo.has_value()) {
                expect(result.serverInfo->name).to_equal(std::string("bpfls"));
            }

            client.initialized({});
            (void)client.shutdown().get();
            client.exit();
            exit_code = process.wait();
        }
        expect(exit_code).to_equal(0);
    });

    it("exits with 1 when told to exit without shutting down", _ {
        auto process = lsp::Process::start(BPFLS_EXECUTABLE);
        lsp::ClientEndpoint client(process.stdIO());
        int exit_code = -1;
        {
            ClientLoop loop(client);

            (void)client.initialize(initialize_params()).get();
            client.initialized({});
            client.exit();
            exit_code = process.wait();
        }
        expect(exit_code).to_equal(1);
    });
});

CPPSPEC_MAIN(lifecycle_test);
