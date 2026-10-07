// Front matter: the small YAML subset used by posts and pages
// This is free and unencumbered software released into the public domain.

struct FrontMatter {
    std::string_view        title;
    std::string_view        layout;
    std::string_view        date;
    std::string_view        uuid;
    std::vector<std::string_view> tags;
    std::string_view        body;     // content after the front matter
    iz         bodyline; // line number where body begins
    b32        ok;
    std::string_view        err;
    iz         errline;
};

// Parse a YAML scalar value: plain, 'single' ('' escapes), "double"
// (only \" and \\ escapes). Returns a null string on syntax error.
static std::string_view yamlscalar(std::string_view v, Arena *perm)
{
    v = trim(v);
    if (v.empty() || (v[0] != '\'' && v[0] != '"')) {
        return std::ssize(v) ? v : std::string_view{"", 0};
    }
    u8  q = v[0];
    Buf b(perm, std::ssize(v));
    for (iz i = 1; i < std::ssize(v); i++) {
        u8 c = v[i];
        if (c == q) {
            if (q=='\'' && i+1<std::ssize(v) && v[i+1]=='\'') {
                putbyte(&b, '\'');
                i++;
                continue;
            }
            if (i != std::ssize(v)-1) {
                return {};  // trailing junk after closing quote
            }
            std::string_view r = finish(&b);
            return std::ssize(r) ? r : std::string_view{"", 0};
        } else if (q=='"' && c=='\\') {
            if (i+1 >= std::ssize(v) || (v[i+1] != '"' && v[i+1] != '\\')) {
                return {};  // unsupported escape
            }
            putbyte(&b, v[++i]);
        } else {
            putbyte(&b, c);
        }
    }
    return {};  // unterminated
}

static b32 tagchar(u8 c)
{
    return (c>='a' && c<='z') || digit(c);
}

static FrontMatter parsefrontmatter(std::string_view src, Arena *perm)
{
    FrontMatter r = {};
    if (!src.starts_with("---\n")) {
        r.err = "missing front matter";
        return r;
    }
    std::string_view rest = src.substr(4);
    iz  line = 1;
    for (;;) {
        line++;
        if (rest.empty()) {
            r.err = "unterminated front matter";
            r.errline = line;
            return r;
        }
        Cut c = cut(rest, '\n');
        rest = c.tail;
        std::string_view ln = trimright(c.head);
        if (ln == "---" || ln == "...") {
            break;
        }
        if (ln.empty()) {
            continue;
        }

        Cut kv = cut(ln, ':');
        std::string_view key = kv.head;
        std::string_view val = trim(kv.tail);
        if (!kv.ok || key.empty() || whitespace(key[0])) {
            r.err = "expected 'key: value'";
            r.errline = line;
            return r;
        }

        if (key == "title") {
            r.title = yamlscalar(val, perm);
            if (!r.title.data()) {
                r.err = "invalid title string";
                r.errline = line;
                return r;
            }
        } else if (key == "layout") {
            r.layout = val;
        } else if (key == "date") {
            r.date = val;
        } else if (key == "uuid") {
            r.uuid = val;
        } else if (key == "excerpt_separator") {
            // Ignored: <!--more--> is always the separator
        } else if (key == "tags") {
            if (std::ssize(val)<2 || val[0]!='[' || val[std::ssize(val)-1]!=']') {
                r.err = "tags must be an inline list: [a, b]";
                r.errline = line;
                return r;
            }
            std::string_view list = trim((val.substr(1)).substr(0, std::ssize(val)-2));
            for (Cut t = {{}, list, std::ssize(list)>0}; t.ok;) {
                t = cut(t.tail, ',');
                std::string_view tag = trim(t.head);
                b32 valid = std::ssize(tag) > 0;
                for (iz i = 0; i < std::ssize(tag); i++) {
                    valid &= tagchar(tag[i]);
                }
                if (!valid) {
                    r.err = "invalid tag name";
                    r.errline = line;
                    return r;
                }
                r.tags.push_back(tag);
            }
        } else {
            r.err = "unknown front matter key";
            r.errline = line;
            return r;
        }
    }

    // Like Jekyll, the blank lines after the closing marker are not
    // part of the body.
    for (;;) {
        Cut c = cut(rest, '\n');
        if (!c.ok || std::ssize(trim(c.head))) {
            break;
        }
        rest = c.tail;
        line++;
    }
    r.body     = rest;
    r.bodyline = line + 1;
    r.ok       = 1;
    return r;
}
