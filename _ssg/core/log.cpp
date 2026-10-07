// Diagnostics: warnings and errors collected into a buffer
// This is free and unencumbered software released into the public domain.

struct Log {
    Buf buf;
    i32 errors   = 0;
    i32 warnings = 0;
};

static void logmsg(Log *log, std::string_view kind, std::string_view file, iz line, std::string_view msg)
{
    Buf *b = &log->buf;
    print(b, "ssg: ");
    if (std::ssize(file)) {
        print(b, file);
        if (line > 0) {
            putbyte(b, ':');
            print(b, (i64)line);
        }
        print(b, ": ");
    }
    print(b, kind);
    print(b, msg);
    putbyte(b, '\n');
}

static void warn(Log *log, std::string_view file, iz line, std::string_view msg)
{
    log->warnings++;
    logmsg(log, "warning: ", file, line, msg);
}

static void error(Log *log, std::string_view file, iz line, std::string_view msg)
{
    log->errors++;
    logmsg(log, "error: ", file, line, msg);
}
