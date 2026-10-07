// hlcat: highlight standard input, printing the HTML to standard output
// $ hlcat LANG <file
// Exits with 1 if LANG is unknown (the output is then plain escaped
// text). Debug builds check that the markup round-trips to the input.
// This is free and unencumbered software released into the public domain.
#include <cstdio>
#include "core/base.cpp"
#include "parser/highlight.cpp"

static b32 writeall(std::FILE *f, std::string_view s)
{
    return std::fwrite(s.data(), 1, s.size(), f) == s.size();
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        writeall(stderr, "usage: hlcat LANG <file\n");
        return 2;
    }
    std::string_view lang = argv[1];

    std::string code;
    char        buf[1<<16];
    for (uz n; (n = std::fread(buf, 1, sizeof(buf), stdin));) {
        code.append(buf, n);
    }

    std::string b;
    b32 known = highlight(&b, lang, code);
    if (!writeall(stdout, b)) {
        return 2;
    }
    return !known;
}
