// Platform layer: implements core/os.cpp with standard C++
//
// <filesystem> calls take the std::error_code overload: a file that
// cannot be read is reported and the build goes on, not unwound. Paths
// go through char8_t, so they stay UTF-8 on Windows too.
//
// This is free and unencumbered software released into the public domain.
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <thread>
#include "engine/ssg.cpp"

namespace fs = std::filesystem;

struct Os {};

static fs::path topath(std::string_view s)
{
    return std::u8string_view{(char8_t const *)s.data(), s.size()};
}

static std::string_view os_read(Os *, Arena *a, std::string_view path)
{
    fs::path        p = topath(path);
    std::error_code ec;
    if (!fs::is_regular_file(p, ec)) {
        return {};
    }
    iz size = (iz)fs::file_size(p, ec);
    if (ec) {
        return {};
    }
    std::ifstream f(p, std::ios::binary);
    if (!f) {
        return {};
    }
    char *data = allocbytes(a, size);
    f.read(data, size);
    if (f.gcount() != size) {
        return {};
    }
    return {data, (uz)size};
}

static b32 os_write(Os *, Arena, std::string_view path, std::string_view data)
{
    std::ofstream f(topath(path), std::ios::binary|std::ios::trunc);
    f.write(data.data(), std::ssize(data));
    f.close();
    return !f.fail();
}

static b32 os_mkdirs(Os *, Arena, std::string_view path)
{
    std::error_code ec;
    fs::create_directories(topath(path), ec);
    return !ec;
}

static Dirent *os_list(Os *, Arena *a, std::string_view path)
{
    std::error_code         ec;
    fs::directory_iterator  it(topath(path), ec);
    if (ec) {
        return 0;
    }
    Dirent *head = 0;
    for (; it != fs::directory_iterator{}; it.increment(ec)) {
        fs::file_status st = it->symlink_status(ec);
        if (ec || (!fs::is_directory(st) && !fs::is_regular_file(st))) {
            continue;
        }
        b32 isdir = fs::is_directory(st);
        i64 size  = isdir ? 0 : (i64)it->file_size(ec);
        i64 mtime = (i64)it->last_write_time(ec).time_since_epoch().count();
        if (ec) {
            continue;
        }
        std::u8string name = it->path().filename().u8string();
        Dirent *d = alloc<Dirent>(a);
        d->name  = clone(a, {(char const *)name.data(), name.size()});
        d->size  = size;
        d->mtime = mtime;
        d->isdir = isdir;
        d->next  = head;
        head = d;
    }
    return ec ? 0 : head;
}

static i32 os_copy(Os *, Arena, std::string_view src, std::string_view dst)
{
    fs::path        s = topath(src);
    fs::path        d = topath(dst);
    std::error_code ec;
    auto ssize = fs::file_size(s, ec);
    auto stime = fs::last_write_time(s, ec);
    if (ec) {
        return COPY_FAILED;
    }
    auto dsize = fs::file_size(d, ec);
    auto dtime = fs::last_write_time(d, ec);
    if (!ec && dsize == ssize && dtime >= stime) {
        return COPY_SKIPPED;
    }
    fs::copy_file(s, d, fs::copy_options::overwrite_existing, ec);
    return ec ? COPY_FAILED : COPY_DONE;
}

[[noreturn]] static void os_oom()
{
    std::fputs("ssg: out of memory\n", stderr);
    std::_Exit(2);
}

static i64 os_now(Os *)
{
    auto t = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(t).count();
}

static i64 os_clock(Os *)
{
    auto t = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(t).count();
}

static b32 os_print(Os *, i32 fd, std::string_view s)
{
    std::FILE *f = fd==1 ? stdout : stderr;
    b32 ok = std::fwrite(s.data(), 1, s.size(), f) == s.size();
    return ok && !std::fflush(f);
}

static void os_sleep(Os *, i32 ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

int main(int argc, char **argv)
{
    // Untouched pages are not backed by memory, as with mmap.
    iz    cap = (iz)1<<28;
    byte *mem = (byte *)std::malloc((uz)cap);
    if (!mem) {
        return 2;
    }
    Arena a = {mem, mem+cap};

    std::string_view *args = alloc<std::string_view>(&a, argc);
    for (i32 i = 0; i < argc; i++) {
        args[i] = argv[i];
    }

    Os os;
    return ssg_main(&os, a, args, argc);
}
