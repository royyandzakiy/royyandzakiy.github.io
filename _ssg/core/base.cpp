// Foundation: types, arenas, strings, buffers
// This is free and unencumbered software released into the public domain.
#include <algorithm>
#include <chrono>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <functional>
#include <iterator>
#include <new>
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
using byte = char;

#define assert(c)   while (!(c)) __builtin_unreachable()

struct Arena {
    byte *beg = {};
    byte *end = {};
};

// Provided by the platform layer: report out-of-memory and exit.
[[noreturn]] static void os_oom();

template<typename T>
static T *alloc(Arena *a, iz count = 1)
{
    iz size = sizeof(T);
    iz pad  = -(uz)a->beg & (alignof(T) - 1);
    if (count >= (a->end - a->beg - pad)/size) {
        os_oom();
    }
    T *r = (T *)(a->beg + pad);
    a->beg += pad + count*size;
    for (iz i = 0; i < count; i++) {
        new (r+i) T();
    }
    return r;
}

// Like alloc<char>, but does not zero the memory.
static char *allocbytes(Arena *a, iz count)
{
    if (count > a->end - a->beg) {
        os_oom();
    }
    char *r = a->beg;
    a->beg += count;
    return r;
}

static void copybytes(void *dst, void const *src, iz len)
{
    if (len) std::memcpy(dst, src, (uz)len);
}


// Strings: std::string_view over arena or literal memory. A null view
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

static std::string_view clone(Arena *a, std::string_view s)
{
    char *r = allocbytes(a, std::ssize(s));
    copybytes(r, s.data(), std::ssize(s));
    return {r, s.size()};
}

// Concatenate, extending head in place when it sits at the arena's end.
static std::string_view concat(Arena *a, std::string_view head, std::string_view tail)
{
    if (!head.data() || head.data()+head.size() != a->beg) {
        head = clone(a, head);
    }
    clone(a, tail);
    return {head.data(), head.size()+tail.size()};
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


// Output buffers: appending grows in the arena, in place when possible.

struct Buf {
    char  *data = {};
    iz     len  = {};
    iz     cap  = {};
    Arena *a    = {};

    Buf() = default;
    Buf(Arena *a, iz cap = 1<<12) : data{allocbytes(a, cap)}, cap{cap}, a{a} {}

    // For std::back_inserter, so std::format_to can append
    using value_type = char;
    void push_back(char c);
};

static void reserve(Buf *b, iz need)
{
    if (b->cap - b->len >= need) {
        return;
    }
    Arena *a = b->a;
    iz extend = b->cap > need ? b->cap : need;
    if ((byte *)(b->data+b->cap) == a->beg) {
        allocbytes(a, extend);
    } else {
        char *data = allocbytes(a, b->cap+extend);
        copybytes(data, b->data, b->len);
        b->data = data;
    }
    b->cap += extend;
}

static void print(Buf *b, std::string_view s)
{
    reserve(b, std::ssize(s));
    copybytes(b->data+b->len, s.data(), std::ssize(s));
    b->len += std::ssize(s);
}

static void putbyte(Buf *b, u8 c)
{
    reserve(b, 1);
    b->data[b->len++] = (char)c;
}

void Buf::push_back(char c)
{
    putbyte(this, (u8)c);
}

// std::format into the buffer
template<typename... Args>
static void printfmt(Buf *b, std::format_string<Args...> fmt, Args &&...args)
{
    std::format_to(std::back_inserter(*b), fmt, std::forward<Args>(args)...);
}

static void print(Buf *b, i64 v)
{
    printfmt(b, "{}", v);
}

// Zero-padded decimal of at least width digits.
static void printpad(Buf *b, i64 v, i32 width)
{
    printfmt(b, "{:0{}}", v, width);
}

// The low digits hex digits of v, zero-padded.
static void printhex(Buf *b, u64 v, i32 digits)
{
    u64 mask = digits<16 ? ((u64)1<<(4*digits)) - 1 : ~(u64)0;
    printfmt(b, "{:0{}x}", v & mask, digits);
}

// Escape & < > for HTML text content.
static void printhtml(Buf *b, std::string_view s)
{
    iz last = 0;
    for (iz i = 0; i < std::ssize(s); i++) {
        std::string_view rep;
        switch (s[i]) {
        case '&': rep = "&amp;"; break;
        case '<': rep = "&lt;";  break;
        case '>': rep = "&gt;";  break;
        default: continue;
        }
        print(b, s.substr(last, i-last));
        print(b, rep);
        last = i + 1;
    }
    print(b, s.substr(last));
}

// Escape & < > " for double-quoted HTML attributes.
static void printattr(Buf *b, std::string_view s)
{
    iz last = 0;
    for (iz i = 0; i < std::ssize(s); i++) {
        std::string_view rep;
        switch (s[i]) {
        case '&': rep = "&amp;";  break;
        case '<': rep = "&lt;";   break;
        case '>': rep = "&gt;";   break;
        case '"': rep = "&quot;"; break;
        default: continue;
        }
        print(b, s.substr(last, i-last));
        print(b, rep);
        last = i + 1;
    }
    print(b, s.substr(last));
}

static std::string_view finish(Buf *b)
{
    return {b->data, (uz)b->len};
}
