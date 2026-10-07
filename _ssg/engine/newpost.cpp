// New posts: turn a loose Markdown draft into a dated post in _posts
// This is free and unencumbered software released into the public domain.

struct Draft {
    std::string_view title;
    std::string_view body;
};

// The draft begins with a "# Title" heading, which is removed from the
// body along with surrounding blank lines. The title is empty when the
// first non-blank line is not such a heading.
static Draft parsedraft(std::string_view src)
{
    Draft r    = {};
    std::string_view   rest = src;
    for (;;) {
        Cut c    = cut(rest, '\n');
        std::string_view line = trim(c.head);
        if (line.empty()) {
            if (!c.ok) {
                return r;
            }
            rest = c.tail;
            continue;
        }
        if (std::ssize(line)<2 || line[0]!='#' || !whitespace(line[1])) {
            return r;
        }
        std::string_view title = trim(line.substr(1));
        iz  n     = std::ssize(title);
        for (; n && title[n-1]=='#'; n--) {}
        if (!n || whitespace(title[n-1])) {
            title = trim(title.substr(0, n));  // closing hashes
        }
        r.title = title;
        rest    = c.tail;
        break;
    }

    for (;;) {
        Cut c = cut(rest, '\n');
        if (!c.ok || std::ssize(trim(c.head))) {
            break;
        }
        rest = c.tail;
    }
    r.body = trimright(rest);
    return r;
}

// Runs of non-alphanumerics become a dash. A leading dash is dropped, but
// a trailing dash is kept, as in existing names like "...-in-C-" (C++).
static void printslug(Buf *b, std::string_view title)
{
    b32 dash = 0;
    b32 any  = 0;
    for (iz i = 0; i < std::ssize(title); i++) {
        u8 c = title[i];
        if (!alnum(c)) {
            dash = 1;
            continue;
        }
        if (dash && any) {
            putbyte(b, '-');
        }
        putbyte(b, c);
        dash = 0;
        any  = 1;
    }
    if (dash && any) {
        putbyte(b, '-');
    }
}

// A YAML scalar: plain when safe, otherwise single-quoted.
static void printyaml(Buf *b, std::string_view s)
{
    b32 quote = s.empty() || s[std::ssize(s)-1]==':' || !(trim(s) == s) ||
                s.contains(": ") || s.contains(" #") ||
                std::string_view("-?:,[]{}#&*!|>'\"%@`").contains(s[0]);
    if (!quote) {
        print(b, s);
        return;
    }
    putbyte(b, '\'');
    for (iz i = 0; i < std::ssize(s); i++) {
        if (s[i] == '\'') {
            putbyte(b, '\'');
        }
        putbyte(b, s[i]);
    }
    putbyte(b, '\'');
}

static std::string_view postname(Arena *perm, std::string_view title, i64 now)  // or empty
{
    Buf name(perm, 64+std::ssize(title));
    printymd(&name, now);
    putbyte(&name, '-');
    iz prefix = name.len;
    printslug(&name, title);
    if (name.len == prefix) {
        return {};
    }
    print(&name, ".markdown");
    return finish(&name);
}

// Write the draft into _posts, dated now, and print its path.
static i32 newpost(Os *os, Options *opt, Log *log, Arena *perm, Arena scratch)
{
    std::string_view draft = os_read(os, perm, opt->newpost);
    if (!draft.data()) {
        error(log, opt->newpost, 0, "could not read file");
        return 1;
    }
    Draft d = parsedraft(normalize(perm, draft));
    if (d.title.empty()) {
        error(log, opt->newpost, 0, "draft must begin with a \"# Title\" heading");
        return 1;
    }
    std::string_view name = postname(perm, d.title, opt->now);
    if (name.empty()) {
        error(log, opt->newpost, 0, "no usable file name from the title");
        return 1;
    }

    // Post URLs are by date, so one post per day
    std::string_view dir = join(perm, opt->src, "_posts");
    std::string_view day = name.substr(0, 11);  // "YYYY-MM-DD-"
    for (Dirent *e = os_list(os, &scratch, dir); e; e = e->next) {
        if (e->name.starts_with(day)) {
            error(log, join(perm, dir, e->name), 0, "a post already has this date");
            return 1;
        }
    }

    std::string_view uuid = deriveuuid(perm, name);
    std::string_view path = join(perm, dir, name);
    Buf b(&scratch, 1<<12);
    print(&b, "---\ntitle: ");
    printyaml(&b, d.title);
    print(&b, "\ndate: ");
    printiso(&b, opt->now);
    print(&b, "\ntags: []\nuuid: ");
    print(&b, uuid);
    print(&b, "\n---\n\n");
    print(&b, d.body);
    putbyte(&b, '\n');
    std::string_view post = finish(&b);
    if (!os_write(os, scratch, path, post)) {
        error(log, path, 0, "could not write file");
        return 1;
    }

    Buf out(&scratch, std::ssize(path)+1);
    print(&out, path);
    putbyte(&out, '\n');
    os_print(os, 1, finish(&out));
    return 0;
}
