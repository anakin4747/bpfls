#include <lsp/io/standard_io.h>

#include "server.h"

int main()
{
    return bpfls::Server(lsp::io::standardIO()).run();
}
