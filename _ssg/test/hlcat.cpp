// hlcat: highlight standard input, printing the HTML to standard output
// $ hlcat LANG <file
// Exits with 1 if LANG is unknown (the output is then plain escaped
// text). Debug builds check that the markup round-trips to the input.
// This is free and unencumbered software released into the public domain.
#include <cstdio>
#include <cstdlib>
#include "core/base.cpp"
#include "parser/highlight.cpp"

static b32 writeall(std::FILE *f, Str s)
{
    return std::fwrite(s.data, 1, (uz)s.len, f) == (uz)s.len;
}

[[noreturn]] static void os_oom()
{
    writeall(stderr, "hlcat: out of memory\n");
    std::_Exit(2);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        writeall(stderr, "usage: hlcat LANG <file\n");
        return 2;
    }
    Str lang = {(u8 *)argv[1], 0};
    while (lang.data[lang.len]) lang.len++;

    iz    cap = (iz)1<<28;
    byte *mem = (byte *)std::malloc((uz)cap);
    if (!mem) {
        return 2;
    }
    Arena perm    = {mem, mem+cap/2};
    Arena scratch = {mem+cap/2, mem+cap};

    Str code = {};
    code.data = allocbytes(&perm, cap/4);
    code.len  = (iz)std::fread(code.data, 1, (uz)(cap/4), stdin);
    perm.beg = (byte *)(code.data + code.len);

    Buf b(&perm, code.len*2 + 64);
    b32 known = highlight(&b, lang, code, scratch);
    if (!writeall(stdout, finish(&b))) {
        return 2;
    }
    return !known;
}
