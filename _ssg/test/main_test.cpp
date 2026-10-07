// Unit tests: a platform layer with stubbed I/O
// $ cmake --build build && ./build/ssg_test
// This is free and unencumbered software released into the public domain.
#include <cstdio>
#include "engine/ssg.cpp"

struct Os {};
static std::optional<std::string> os_read(Os *, std::string_view) { return {}; }
static b32 os_write(Os *, std::string_view, std::string_view) { return 0; }
static b32 os_mkdirs(Os *, std::string_view) { return 0; }
static std::vector<Dirent> os_list(Os *, std::string_view) { return {}; }
static i32 os_copy(Os *, std::string_view, std::string_view) { return COPY_FAILED; }
static i64 os_now(Os *) { return 0; }
static i64 os_clock(Os *) { return 0; }
static b32 os_print(Os *, i32 fd, std::string_view s)
{
    return std::fwrite(s.data(), 1, s.size(), fd==1 ? stdout : stderr) == s.size();
}
static void os_sleep(Os *, i32) {}

struct Test {
    i32 run;
    i32 failed;
};

static void printstr(FILE *f, std::string_view s)
{
    fwrite(s.data(), 1, s.size(), f);
}

// Compare strings, printing both on mismatch.
static b32 expect(Test *t, std::string_view name, std::string_view got, std::string_view want)
{
    t->run++;
    if (got == want) {
        return 1;
    }
    t->failed++;
    fprintf(stderr, "FAIL: ");
    printstr(stderr, name);
    fprintf(stderr, "\n  got:  [");
    printstr(stderr, got);
    fprintf(stderr, "]\n  want: [");
    printstr(stderr, want);
    fprintf(stderr, "]\n");
    return 0;
}

#include "site_test.cpp"
#include "newpost_test.cpp"
#include "markdown_test.cpp"
#include "highlight_test.cpp"

int main()
{
    Test t = {};

    test_site(&t);
    test_newpost(&t);
    test_markdown(&t);
    test_highlight(&t);

    fprintf(stderr, "%d tests, %d failed\n", t.run, t.failed);
    return t.failed ? 1 : 0;
}
