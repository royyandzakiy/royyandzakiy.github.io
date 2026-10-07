// Diagnostics: warnings and errors collected into a buffer
// This is free and unencumbered software released into the public domain.

struct Log {
    Buf buf;
    i32 errors   = 0;
    i32 warnings = 0;
};

static void logmsg(Log *log, Str kind, Str file, iz line, Str msg)
{
    Buf *b = &log->buf;
    print(b, "ssg: ");
    if (file.len) {
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

static void warn(Log *log, Str file, iz line, Str msg)
{
    log->warnings++;
    logmsg(log, "warning: ", file, line, msg);
}

static void error(Log *log, Str file, iz line, Str msg)
{
    log->errors++;
    logmsg(log, "error: ", file, line, msg);
}
