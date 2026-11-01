#include <string>

// cppspec defines macros such as `it`, which clash with identifiers in
// bpftrace's headers, so project headers must be included first.
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
});

CPPSPEC_MAIN(server_test);
