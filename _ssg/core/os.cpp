// Platform API: implemented by each platform layer (platform/, test/main_test.cpp)
// This is free and unencumbered software released into the public domain.

struct Os;

struct Dirent {
    Dirent *next;
    std::string_view     name;
    i64     size;
    i64     mtime;  // platform units, only compared for changes
    b32     isdir;
};

enum {
    COPY_FAILED,
    COPY_DONE,
    COPY_SKIPPED,
};

// Read an entire file into the arena. Returns a null string on failure.
static std::string_view     os_read(Os *, Arena *, std::string_view path);
// Create or truncate a file with the given contents.
static b32     os_write(Os *, Arena, std::string_view path, std::string_view data);
// Create a directory and all its parents.
static b32     os_mkdirs(Os *, Arena, std::string_view path);
// List a directory, excluding "." and "..", symlinks, and special files.
static Dirent *os_list(Os *, Arena *, std::string_view path);
// Copy a file unless the destination has same size and is not older.
static i32     os_copy(Os *, Arena, std::string_view src, std::string_view dst);
// Current time in Unix epoch seconds.
static i64     os_now(Os *);
// Monotonic clock in milliseconds, only for measuring intervals.
static i64     os_clock(Os *);
// Write to standard output (1) or standard error (2).
static b32     os_print(Os *, i32 fd, std::string_view);
// Pause for a number of milliseconds.
static void    os_sleep(Os *, i32 ms);
// Also: [[noreturn]] os_oom(), declared in core/base.cpp.
