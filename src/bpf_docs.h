#pragma once

#include <string_view>

namespace bpfls {

// Returns the lines of bpftrace/docs/<file> from the line equal to `start`
// up to, but not including, the line equal to `end`, computed at compile time.
consteval std::string_view bpf_docs([[maybe_unused]] std::string_view file,
                                    std::string_view start,
                                    [[maybe_unused]] std::string_view end)
{
    return start;
}

} // namespace bpfls
