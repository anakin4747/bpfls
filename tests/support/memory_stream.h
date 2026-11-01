#pragma once

#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include <lsp/io/stream.h>
#include <lsp/json/json.h>

namespace bpfls::test {

// An lsp::io::Stream backed by strings, so a server can be driven entirely
// in memory: reads come from a fixed input and writes are collected for
// inspection. Once the input runs out, reads report end of stream, which ends
// the server's message loop.
class MemoryStream : public lsp::io::Stream {
public:
    explicit MemoryStream(std::string input) : input_(std::move(input))
    {
    }

    void read(char *buffer, std::size_t size) override
    {
        if (size == 0)
            return;
        if (offset_ + size > input_.size()) {
            if (size == 1) {
                *buffer = Eof;
                return;
            }
            throw lsp::io::Error("MemoryStream: read past the end of input");
        }
        std::memcpy(buffer, input_.data() + offset_, size);
        offset_ += size;
    }

    void write(const char *buffer, std::size_t size) override
    {
        output_.append(buffer, size);
    }

    [[nodiscard]] const std::string &output() const
    {
        return output_;
    }

private:
    std::string input_;
    std::size_t offset_ = 0;
    std::string output_;
};

// Wraps a JSON-RPC message body in the LSP base protocol header.
inline std::string frame(std::string_view body)
{
    return "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" +
           std::string(body);
}

// Splits LSP base protocol output into its parsed JSON messages.
inline std::vector<lsp::json::Value> messages(std::string_view output)
{
    constexpr std::string_view length_header = "Content-Length: ";
    constexpr std::string_view separator = "\r\n\r\n";

    std::vector<lsp::json::Value> result;
    std::size_t pos = 0;
    while ((pos = output.find(length_header, pos)) != std::string_view::npos) {
        pos += length_header.size();
        std::size_t length = std::stoul(std::string(output.substr(pos)));
        pos = output.find(separator, pos) + separator.size();
        result.push_back(lsp::json::parse(output.substr(pos, length)));
        pos += length;
    }
    return result;
}

} // namespace bpfls::test
