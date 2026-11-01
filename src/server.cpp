#include "server.h"

namespace bpfls {

Server::Server(lsp::io::Stream &stream) : endpoint_(stream)
{
}

int Server::run()
{
    return -1;
}

} // namespace bpfls
