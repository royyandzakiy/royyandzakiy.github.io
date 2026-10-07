// Diagnostics: warnings and errors collected into a buffer
// This is free and unencumbered software released into the public domain.

struct Log {
    std::string buf;
    i32 errors   = 0;
    i32 warnings = 0;
};

static void logmsg(Log *log, std::string_view kind, std::string_view file, iz line, std::string_view msg)
{
    std::string *b = &log->buf;
    *b += "ssg: ";
    if (std::ssize(file)) {
        *b += file;
        if (line > 0) {
            *b += ':';
            *b += std::to_string(line);
        }
        *b += ": ";
    }
    *b += kind;
    *b += msg;
    *b += '\n';
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
