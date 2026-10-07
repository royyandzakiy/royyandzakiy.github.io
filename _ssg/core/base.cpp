// Foundation: types, strings, output
// This is free and unencumbered software released into the public domain.
#include <algorithm>
#include <chrono>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <format>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using u8   = std::uint8_t;
using b32  = std::int32_t;
using i32  = std::int32_t;
using u32  = std::uint32_t;
using i64  = std::int64_t;
using u64  = std::uint64_t;
using iz   = std::ptrdiff_t;
using uz   = std::size_t;

#define assert(c)   while (!(c)) __builtin_unreachable()

// Strings: std::string_view to read, std::string to own. A null view
// (data() == nullptr) means "none", distinct from an empty string.
// Bytes are read as u8, since char may be signed and UTF-8 must compare
// unsigned; std::string_view's own comparisons already are (like memcmp).

// Split at the first c. Without one, tail is empty and ok is 0.
struct Cut {
    std::string_view head;
    std::string_view tail;
    b32              ok;
};

static Cut cut(std::string_view s, char c)
{
    uz i = s.find(c);
    if (i == s.npos) {
        return {s, s.substr(s.size()), 0};
    }
    return {s.substr(0, i), s.substr(i+1), 1};
}

static b32 whitespace(u8 c)
{
    return c==' ' || c=='\t' || c=='\n' || c=='\r' || c=='\f' || c=='\v';
}

static b32 digit(u8 c)
{
    return c>='0' && c<='9';
}

static b32 letter(u8 c)
{
    return (c>='a' && c<='z') || (c>='A' && c<='Z');
}

static b32 alnum(u8 c)
{
    return letter(c) || digit(c);
}

static u8 lowercase(u8 c)
{
    return c>='A' && c<='Z' ? (u8)(c + 32) : c;
}

static std::string_view trimleft(std::string_view s)
{
    while (!s.empty() && whitespace(s.front())) s.remove_prefix(1);
    return s;
}

static std::string_view trimright(std::string_view s)
{
    while (!s.empty() && whitespace(s.back())) s.remove_suffix(1);
    return s;
}

static std::string_view trim(std::string_view s)
{
    return trimleft(trimright(s));
}

static u64 hash(std::string_view s)
{
    u64 h = 0x100;
    for (u8 c : s) {
        h ^= c;
        h *= 1111111111111111111u;
    }
    return h;
}

// Decode one UTF-8 code point at s[*i], advancing *i. Invalid bytes
// decode as themselves (Latin-1), one byte at a time.
static i32 utf8decode(std::string_view s, iz *i)
{
    u8  c = s[(*i)++];
    i32 n = c>=0xf0 ? 3 : c>=0xe0 ? 2 : c>=0xc0 ? 1 : 0;
    if (!n || *i+n > std::ssize(s)) {
        return c;
    }
    i32 r = c & (0x3f >> n);
    for (i32 k = 0; k < n; k++) {
        u8 b = s[*i+k];
        if ((b & 0xc0) != 0x80) {
            return c;
        }
        r = r<<6 | (b & 0x3f);
    }
    *i += n;
    return r;
}

// Code point ending just before s[i], or -1 at the start.
static i32 utf8prev(std::string_view s, iz i, iz *start)
{
    if (i <= 0) {
        *start = 0;
        return -1;
    }
    iz j = i - 1;
    for (i32 k = 0; k < 3 && j > 0 && ((u8)s[j]&0xc0) == 0x80; k++, j--) {}
    iz end = j;
    i32 r = utf8decode(s, &end);
    if (end != i) {
        j = i - 1;
        r = (u8)s[j];
    }
    *start = j;
    return r;
}


// Output accumulates in std::string.

// std::format onto the end of b
template<typename... Args>
static void printfmt(std::string *b, std::format_string<Args...> fmt, Args &&...args)
{
    std::format_to(std::back_inserter(*b), fmt, std::forward<Args>(args)...);
}

// The low digits hex digits of v, zero-padded.
static void printhex(std::string *b, u64 v, i32 digits)
{
    u64 mask = digits<16 ? ((u64)1<<(4*digits)) - 1 : ~(u64)0;
    printfmt(b, "{:0{}x}", v & mask, digits);
}

// Append s with each character in special replaced by its entity,
// copying the runs between them whole.
static void printescaped(std::string *b, std::string_view s, std::string_view special)
{
    for (uz i; (i = s.find_first_of(special)) != s.npos; s.remove_prefix(i+1)) {
        b->append(s, 0, i);
        switch (s[i]) {
        case '&': *b += "&amp;";  break;
        case '<': *b += "&lt;";   break;
        case '>': *b += "&gt;";   break;
        case '"': *b += "&quot;"; break;
        }
    }
    *b += s;
}

// Escape & < > for HTML text content.
static void printhtml(std::string *b, std::string_view s)
{
    printescaped(b, s, "&<>");
}

// Escape & < > " for double-quoted HTML attributes.
static void printattr(std::string *b, std::string_view s)
{
    printescaped(b, s, "&<>\"");
}
