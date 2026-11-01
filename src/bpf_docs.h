#pragma once

// Sections of bpftrace's documentation (bpftrace/docs/), built into the
// binary at compile time so hover text needs no files at runtime.

#include <array>
#include <cstddef>
#include <string_view>

namespace bpfls {

namespace detail {

// #embed yields byte values 0-255, which cannot initialize a char array
// without narrowing, so the files are embedded as unsigned char and copied
// into char arrays at compile time.
template <std::size_t N>
consteval std::array<char, N> as_chars(const unsigned char (&bytes)[N])
{
    std::array<char, N> chars{};
    for (std::size_t i = 0; i < N; ++i)
        chars[i] = static_cast<char>(bytes[i]);
    return chars;
}

// #embed is a GCC extension before C++26.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc++26-extensions"

inline constexpr unsigned char language_md_bytes[] = {
#embed "../bpftrace/docs/language.md"
};

inline constexpr unsigned char stdlib_md_bytes[] = {
#embed "../bpftrace/docs/stdlib.md"
};

#pragma GCC diagnostic pop

inline constexpr auto language_md = as_chars(language_md_bytes);
inline constexpr auto stdlib_md = as_chars(stdlib_md_bytes);

// The embedded docs file called `file`.
consteval std::string_view docs_file(std::string_view file)
{
    if (file == "language.md")
        return { language_md.data(), language_md.size() };
    if (file == "stdlib.md")
        return { stdlib_md.data(), stdlib_md.size() };
    throw "bpf_docs: unknown docs file; embed it in bpf_docs.h";
}

// Offset of the line in `text` that is exactly `line`, searching from
// `from`, or npos.
consteval std::size_t find_line(std::string_view text,
                                std::string_view line,
                                std::size_t from)
{
    for (std::size_t pos = from; pos < text.size();) {
        std::size_t eol = text.find('\n', pos);
        if (eol == std::string_view::npos)
            eol = text.size();
        if (text.substr(pos, eol - pos) == line)
            return pos;
        pos = eol + 1;
    }
    return std::string_view::npos;
}

} // namespace detail

// Returns the lines of bpftrace/docs/<file> from the line equal to `start`
// up to, but not including, the line equal to `end`. Runs at compile time;
// a missing file or marker line is a compile error.
consteval std::string_view bpf_docs(std::string_view file,
                                    std::string_view start,
                                    std::string_view end)
{
    std::string_view text = detail::docs_file(file);

    std::size_t first = detail::find_line(text, start, 0);
    if (first == std::string_view::npos)
        throw "bpf_docs: start line not found";

    std::size_t last = detail::find_line(text, end, first + start.size());
    if (last == std::string_view::npos)
        throw "bpf_docs: end line not found";

    return text.substr(first, last - first);
}

} // namespace bpfls
