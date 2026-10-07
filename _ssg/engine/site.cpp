// Site model: posts, tags, pages, static files, and the build driver
// This is free and unencumbered software released into the public domain.

struct Options {
    std::string_view src;
    std::string_view out;
    std::string_view mdfile;
    std::string_view newpost;
    i64 now;
    b32 fixedtime;
    b32 excerpts;
    b32 quiet;
    b32 watch;
};

static i64 parseint(std::string_view s)
{
    i64 r = 0;
    b32 neg = std::ssize(s) && s[0]=='-';
    for (iz i = neg; i < std::ssize(s) && digit(s[i]); i++) {
        r = r*10 + (s[i] - '0');
    }
    return neg ? -r : r;
}


// Dates (proleptic Gregorian, UTC), as Unix seconds

namespace chr = std::chrono;

static chr::sys_seconds systime(i64 t)
{
    return chr::sys_seconds{chr::seconds{t}};
}

// Parse fixed-width digits, or -1 on error.
static i32 parsedigits(std::string_view s, iz off, iz len)
{
    if (off+len > std::ssize(s)) {
        return -1;
    }
    i32 r = 0;
    for (iz i = off; i < off+len; i++) {
        if (!digit(s[i])) return -1;
        r = r*10 + (s[i] - '0');
    }
    return r;
}

// Parse "YYYY-MM-DD" at the start of s into a time, or -1.
static i64 parseday(std::string_view s)
{
    i32 y = parsedigits(s, 0, 4);
    i32 m = parsedigits(s, 5, 2);
    i32 d = parsedigits(s, 8, 2);
    if (y<0 || m<1 || m>12 || d<1 || d>31 || s[4]!='-' || s[7]!='-') {
        return -1;
    }
    // Like Jekyll, a day past the end of the month rolls over (Feb 30 = Mar 2)
    chr::year_month_day ymd{chr::year{y}, chr::month{(u32)m}, chr::day{(u32)d}};
    return chr::sys_seconds{chr::sys_days{ymd}}.time_since_epoch().count();
}

// Parse "YYYY-MM-DDTHH:MM:SSZ" into a time, or -1.
static i64 parsetimestamp(std::string_view s)
{
    if (std::ssize(s) != 20 || s[10]!='T' || s[13]!=':' || s[16]!=':' || s[19]!='Z') {
        return -1;
    }
    i64 day = parseday(s);
    i32 h   = parsedigits(s, 11, 2);
    i32 m   = parsedigits(s, 14, 2);
    i32 sec = parsedigits(s, 17, 2);
    if (day<0 || h<0 || h>23 || m<0 || m>59 || sec<0 || sec>60) {
        return -1;
    }
    return day + h*3600 + m*60 + sec;
}

// Without a locale argument, names are the C locale's (English).

static void printymd(std::string *b, i64 t)
{
    printfmt(b, "{:%Y-%m-%d}", systime(t));
}

static void printlongdate(std::string *b, i64 t)
{
    printfmt(b, "{:%B %d, %Y}", systime(t));
}

static void printiso(std::string *b, i64 t)
{
    printfmt(b, "{:%Y-%m-%dT%H:%M:%SZ}", systime(t));
}

static void printrfc822(std::string *b, i64 t)
{
    printfmt(b, "{:%a, %d %b %Y %H:%M:%S GMT}", systime(t));
}


// URL encoders matching Liquid's uri_escape and url_encode filters

static void percent(std::string *b, u8 c)
{
    *b += '%';
    *b += "0123456789ABCDEF"[c>>4];
    *b += "0123456789ABCDEF"[c&15];
}

// Addressable::URI.normalize_component: keep unreserved and reserved.
// Output is escaped for use inside an HTML attribute.
static void printuriescape(std::string *b, std::string_view s)
{
    for (iz i = 0; i < std::ssize(s); i++) {
        u8 c = s[i];
        if (c == '&') {
            *b += "&amp;";
        } else if (alnum(c) || (c && c<0x80 && std::string_view("-._~:/?#[]@!$'()*+,;=").contains((char)c))) {
            *b += c;
        } else {
            percent(b, c);
        }
    }
}

// CGI.escape: keep A-Za-z0-9 _.-~, space as +.
static void printurlencode(std::string *b, std::string_view s)
{
    for (iz i = 0; i < std::ssize(s); i++) {
        u8 c = s[i];
        if (alnum(c) || c=='_' || c=='.' || c=='-' || c=='~') {
            *b += c;
        } else if (c == ' ') {
            *b += '+';
        } else {
            percent(b, c);
        }
    }
}


// Tag feed UUIDs. Tags not listed get a UUID derived from their name.

struct TagUuid {
    std::string_view name;
    std::string_view uuid;
};

static TagUuid taguuids[] = {
    {"ai",           "bbcac281-4091-4371-aaed-1061fd43ac26"},
    {"asyncio",      "702ad32f-4903-4692-9b8c-b8182b548416"},
    {"bsd",          "5e43aa24-aef5-4f63-b37e-42b2d7859c01"},
    {"c",            "0ba7b921-5597-4fbc-8ceb-88afb378c637"},
    {"clojure",      "cf08ecce-d0a4-4d39-a3fc-aacc66a4cf9b"},
    {"compression",  "1c53dce4-4b3d-4f09-8857-46c6a45c2c7b"},
    {"compsci",      "73aad984-a0c4-4870-b941-b92ebee6efda"},
    {"cpp",          "bdc867e4-b8f7-4cf0-8437-adafc8297129"},
    {"crypto",       "799d7efd-3418-409f-b602-5191d8b4fa4c"},
    {"debian",       "31ca31c3-9fcb-4be8-8278-f5e1373a4738"},
    {"elfeed",       "7a1e551e-16c1-4dc3-b77b-861a81651c99"},
    {"elisp",        "8f0d2f56-ad2a-4f37-b61c-16d975ffd79e"},
    {"emacs",        "3d01fe0a-7c1c-475c-b07c-47b7b19e8870"},
    {"fiction",      "88bc4ba3-35fa-4614-8dc1-4ccaf702b655"},
    {"game",         "da57a040-05d6-4aba-ae11-162617d3e9c4"},
    {"git",          "c95ccfd4-9b1b-4f28-877f-8f71751a9dde"},
    {"go",           "51c2a18d-6ae7-4e0c-b14e-a1424eb21dba"},
    {"gpgpu",        "e9f7d39e-ac47-4b56-91c1-796bd01500ad"},
    {"interactive",  "82f38864-6463-4eac-8a86-3eca11fc4fa5"},
    {"java",         "8bee72c7-ad28-4fe4-b67d-ca24449a1c60"},
    {"javascript",   "9d102329-9ecd-4318-8e36-179ecc067f75"},
    {"lang",         "a6275ef4-a851-4550-918f-4cfc74bc9568"},
    {"link",         "8c68d9f0-b18d-4cc3-a43c-3af369f36938"},
    {"linux",        "9d7de7b3-b357-464b-a963-da196bdd5954"},
    {"lisp",         "dffd8000-5e5e-41d1-ab8e-ee2a25ee30a9"},
    {"lua",          "8681fa33-3a8a-46e2-ba71-b2d6bb236a5f"},
    {"math",         "699ac152-8662-474a-aeb6-98c1ddd9febb"},
    {"meatspace",    "22d16006-121f-438a-be7a-f0619eca48b9"},
    {"media",        "2f2627c3-6116-413b-ba68-3a7a0cfeb8fb"},
    {"meta",         "22ce0838-6736-4d87-a83f-0f86c7f1d970"},
    {"netsec",       "7c40636f-98cb-4dbf-b2a1-ed73f8712af7"},
    {"octave",       "ae7db2fb-eac3-4621-bae3-876c87011475"},
    {"opengl",       "90c6162f-52ec-4018-b490-acd91621dc9a"},
    {"openpgp",      "f29b12d7-84e6-4cef-ab71-46e854ab1df6"},
    {"optimization", "6022e81a-277c-4339-b204-59145c368baf"},
    {"perl",         "dd44fb39-63bf-4290-8e5f-818e34348c1c"},
    {"posix",        "bba994f0-2941-4a75-b490-04cd48f44829"},
    {"python",       "7a5665a7-6c04-451e-a43c-f7284c9cfcca"},
    {"rant",         "e5fdd2c0-380e-4ce5-8730-bd443601b65f"},
    {"reddit",       "1cbcfbba-f648-40f5-92fa-de92dd9f262b"},
    {"story",        "1f7c4b50-51dd-40c2-b6dc-75c024e2071e"},
    {"trick",        "a2a6e1b2-9a41-443c-83ab-c457836a1a5d"},
    {"tutorial",     "3e3ec37f-9de8-40de-b725-2f1f16b203c8"},
    {"video",        "6ca8997d-4783-46f5-9390-8b615eac5b75"},
    {"vim",          "d06181fc-6127-4f82-ba7e-0ad17bf6a145"},
    {"web",          "01868576-f382-43f9-bde0-c4415f126084"},
    {"webgl",        "cba28fc1-4172-4bd0-87fd-7920c4fb01ee"},
    {"win32",        "33f52ccf-2d02-4f2f-b574-8dd1468b3e7b"},
    {"x86",          "763a3ddc-a1df-4bad-b03e-86513dc3c50c"},
};

// A deterministic version 8 (custom) UUID derived from a name.
static std::string deriveuuid(std::string_view name)
{
    u64 hi = hash(name);
    u64 lo = hash(std::string(name) + "\n");
    hi = (hi & ~(u64)0xf000) | 0x8000;                // version 8
    lo = (lo & ~((u64)3<<62)) | ((u64)2<<62);         // RFC 9562 variant
    std::string b;
    printhex(&b, hi>>32, 8);
    b += '-';
    printhex(&b, hi>>16, 4);
    b += '-';
    printhex(&b, hi, 4);
    b += '-';
    printhex(&b, lo>>48, 4);
    b += '-';
    printhex(&b, lo, 12);
    return b;
}

static b32 validuuid(std::string_view s)
{
    if (std::ssize(s) != 36) {
        return 0;
    }
    for (iz i = 0; i < std::ssize(s); i++) {
        u8  c    = s[i];
        b32 dash = i==8 || i==13 || i==18 || i==23;
        if (dash ? c!='-' : !(digit(c) || (c>='a' && c<='f'))) {
            return 0;
        }
    }
    return 1;
}


// Site model. Posts and pages own their source text; views such as tags
// and the Markdown body point into it, and stay valid since each lives
// behind a unique_ptr that the Site owns.

struct Post {
    std::string                   src;       // path relative to the source root
    std::string                   text;      // the whole file
    std::string                   title;
    std::string_view              uuid;
    std::vector<std::string_view> tags;
    i64                           time;
    std::string                   url;       // /blog/YYYY-MM/slug/
    std::string                   oldurl;    // /YYYY-MM-DD/slug/, redirects to url
    std::string_view              body;      // Markdown
    iz                            bodyline;
    std::string                   html;
    iz                            excerpt;   // length of the excerpt prefix of html
    Post                         *older;
    Post                         *newer;
};

struct Tag {
    std::string_view    name;
    std::string         uuid;
    std::vector<Post *> posts;  // newest first
};

struct Page {
    std::string      src;    // e.g. about/index.md
    std::string      out;    // e.g. about/index.html
    std::string      text;   // the whole file
    std::string      title;
    std::string_view body;
    iz               bodyline;
    std::string      html;
};

struct Site {
    std::vector<std::unique_ptr<Post>> posts;    // newest first
    std::vector<std::unique_ptr<Tag>>  tags;     // sorted by name
    std::vector<std::unique_ptr<Page>> pages;
    std::vector<std::string>           statics;  // relative paths
    i64                                now;
};

struct Ctx {
    Os                                *os;
    Options                           *opt;
    Log                               *log;
    std::set<std::string, std::less<>> outputs;  // paths written so far
    i32                                written;
    i32                                copied;
    i32                                skipped;
};

static std::string join(std::string_view dir, std::string_view name)
{
    if (dir.empty() || dir == ".") {
        return std::string(name);
    }
    return std::format("{}/{}", dir, name);
}

static std::string srcpath(Ctx *c, std::string_view rel)
{
    return join(c->opt->src, rel);
}

// Normalize CRLF line endings in place.
static void normalize(std::string *s)
{
    if (!s->contains('\r')) {
        return;
    }
    iz n = 0;
    for (iz i = 0; i < std::ssize(*s); i++) {
        if ((*s)[i]=='\r' && i+1<std::ssize(*s) && (*s)[i+1]=='\n') {
            continue;
        }
        (*s)[n++] = (*s)[i];
    }
    s->resize(n);
}

static bool postless(Post *a, Post *b)
{
    // Newest first; ties broken by path, as Jekyll does (reversed)
    if (a->time != b->time) {
        return a->time > b->time;
    }
    return a->src > b->src;
}

static bool tagless(Tag *a, Tag *b)
{
    return a->name < b->name;
}

static std::unique_ptr<Post> loadpost(Ctx *c, std::string_view name)
{
    Log        *log = c->log;
    std::string rel = join("_posts", name);

    // YYYY-MM-DD-slug.(markdown|md)
    i64 day = parseday(name);
    b32 md  = name.ends_with(".markdown") || name.ends_with(".md");
    if (day<0 || std::ssize(name)<12 || name[10]!='-' || !md) {
        error(log, rel, 0, "post names must be YYYY-MM-DD-slug.markdown");
        return nullptr;
    }

    std::optional<std::string> text = os_read(c->os, srcpath(c, rel));
    if (!text) {
        error(log, rel, 0, "could not read file");
        return nullptr;
    }

    auto p  = std::make_unique<Post>();
    p->src  = std::move(rel);
    p->text = std::move(*text);
    normalize(&p->text);

    FrontMatter fm = parsefrontmatter(p->text);
    if (!fm.ok) {
        error(log, p->src, fm.errline, fm.err);
        return nullptr;
    }
    if (!fm.title) {
        error(log, p->src, 0, "missing title");
        return nullptr;
    }
    p->title    = std::move(*fm.title);
    p->uuid     = fm.uuid;
    p->tags     = std::move(fm.tags);
    p->body     = fm.body;
    p->bodyline = fm.bodyline;
    p->time     = day;

    if (!fm.layout.empty() && fm.layout != "post") {
        warn(log, p->src, 0, "ignoring layout other than 'post'");
    }
    if (!validuuid(p->uuid)) {
        error(log, p->src, 0, "missing or invalid uuid");
    }
    if (!fm.date.empty()) {
        i64 t = parsetimestamp(fm.date);
        if (t < 0) {
            error(log, p->src, 0, "date must be YYYY-MM-DDTHH:MM:SSZ");
        } else if (t/86400*86400 != day) {
            error(log, p->src, 0, "front matter date does not match file name");
        } else {
            p->time = t;
        }
    }

    // /blog/YYYY-MM/slug/ from the file name. The old Jekyll permalink,
    // /YYYY-MM-DD/slug/, gets a page redirecting here.
    std::string_view slug = name.substr(11);
    slug.remove_suffix(slug.ends_with(".md") ? 3 : 9);
    p->url    = std::format("/blog/{}/{}/", name.substr(0, 7), slug);
    p->oldurl = std::format("/{}/{}/", name.substr(0, 10), slug);
    return p;
}

static void loadposts(Ctx *c, Site *site)
{
    std::unordered_map<std::string_view, Post *> urls;  // keys in posts

    std::string dir = srcpath(c, "_posts");
    for (Dirent &e : os_list(c->os, dir)) {
        if (e.isdir || e.name.empty() || e.name[0]=='.' || e.name.ends_with("~")) {
            continue;
        }
        std::unique_ptr<Post> p = loadpost(c, e.name);
        if (!p) {
            continue;
        }
        Post *&prev = urls[p->url];
        if (prev) {
            error(c->log, p->src, 0, "another post has the same URL");
            continue;
        }
        prev = p.get();
        site->posts.push_back(std::move(p));
    }
    if (site->posts.empty()) {
        error(c->log, dir, 0, "no posts found");  // or could not list it
        return;
    }

    std::ranges::stable_sort(site->posts, postless, &std::unique_ptr<Post>::get);
    for (iz i = 0; i < std::ssize(site->posts); i++) {
        Post *p  = site->posts[i].get();
        p->newer = i > 0 ? site->posts[i-1].get() : nullptr;
        p->older = i+1 < std::ssize(site->posts) ? site->posts[i+1].get() : nullptr;
    }

    // Tags, collected newest first since posts are already sorted
    std::unordered_map<std::string_view, Tag *> tags;
    for (auto &p : site->posts) {
        for (std::string_view name : p->tags) {
            Tag *&t = tags[name];
            if (!t) {
                t = site->tags.emplace_back(std::make_unique<Tag>()).get();
                t->name = name;
                for (TagUuid &u : taguuids) {
                    if (u.name == name) {
                        t->uuid = u.uuid;
                    }
                }
                if (t->uuid.empty()) {
                    t->uuid = deriveuuid(name);
                }
            }
            if (!t->posts.empty() && t->posts.back() == p.get()) {
                warn(c->log, p->src, 0, "duplicate tag");
                continue;
            }
            t->posts.push_back(p.get());
        }
    }
    std::ranges::stable_sort(site->tags, tagless, &std::unique_ptr<Tag>::get);
}

static b32 skipname(std::string_view name)
{
    return name.empty() || name[0]=='.' || name[0]=='_' || name[0]=='#' ||
           name[0]=='~' || name.ends_with("~");
}

// Paths at the top of the source tree that are never published.
static std::string_view excluded[] = {"CLAUDE.md"};

// Is the entry in directory rel (empty at the top) not published?
static b32 skipentry(std::string_view rel, std::string_view name)
{
    if (skipname(name)) {
        return 1;
    }
    return rel.empty() && std::ranges::find(excluded, name) != std::end(excluded);
}

static void walk(Ctx *c, Site *site, std::string_view rel, std::string_view outdir)
{
    std::string dir = rel.empty() ? std::string(c->opt->src) : srcpath(c, rel);
    for (Dirent &e : os_list(c->os, dir)) {
        if (skipentry(rel, e.name)) {
            continue;
        }

        std::string child = join(rel, e.name);
        if (e.isdir) {
            if (child == outdir) {
                continue;  // output directory inside the source tree
            }
            walk(c, site, child, outdir);
            continue;
        }

        if (child.ends_with(".md")) {
            std::optional<std::string> text = os_read(c->os, srcpath(c, child));
            if (!text) {
                error(c->log, child, 0, "could not read file");
                continue;
            }
            normalize(&*text);
            if (text->starts_with("---\n")) {
                auto p  = std::make_unique<Page>();
                p->text = std::move(*text);
                FrontMatter fm = parsefrontmatter(p->text);
                if (!fm.ok) {
                    error(c->log, child, fm.errline, fm.err);
                    continue;
                }
                p->out      = child.substr(0, child.size()-3) + ".html";
                p->src      = std::move(child);
                p->title    = fm.title.value_or("");
                p->body     = fm.body;
                p->bodyline = fm.bodyline;
                site->pages.push_back(std::move(p));
                continue;
            }
        }
        site->statics.push_back(std::move(child));
    }
}

// Output directory relative to the source directory, when inside it.
// Backslashes are separators too, for Windows.
static std::string relativeout(Options *opt)
{
    std::string o(opt->out), s(opt->src);
    std::ranges::replace(o, '\\', '/');
    std::ranges::replace(s, '\\', '/');
    std::string_view out = o;
    std::string_view src = s;
    while (out.starts_with("./")) out.remove_prefix(2);
    while (out.ends_with("/"))    out.remove_suffix(1);
    if (src == ".") {
        return std::string(out);
    }
    while (src.ends_with("/")) src.remove_suffix(1);
    if (out.starts_with(src) && out.size()>src.size() && out[src.size()]=='/') {
        return std::string(out.substr(src.size()+1));
    }
    return {};
}

// Directory part of path, "" at the top.
static std::string_view dirname(std::string_view path)
{
    uz slash = path.rfind('/');
    return slash==path.npos ? std::string_view{} : path.substr(0, slash);
}

static void emit(Ctx *c, std::string_view rel, std::string_view data)
{
    if (!c->outputs.emplace(rel).second) {
        error(c->log, rel, 0, "output path generated twice");
        return;
    }

    std::string      path = join(c->opt->out, rel);
    std::string_view dir  = dirname(path);
    if (!dir.empty() && !os_mkdirs(c->os, dir)) {
        error(c->log, path, 0, "could not create directory");
        return;
    }
    if (!os_write(c->os, path, data)) {
        error(c->log, path, 0, "could not write file");
        return;
    }
    c->written++;
}

static void copystatic(Ctx *c, std::string_view rel)
{
    if (c->outputs.contains(rel)) {
        return;  // replaced by a generated file
    }
    std::string      src = srcpath(c, rel);
    std::string      dst = join(c->opt->out, rel);
    std::string_view dir = dirname(dst);
    if (!dir.empty() && !os_mkdirs(c->os, dir)) {
        error(c->log, dst, 0, "could not create directory");
        return;
    }
    switch (os_copy(c->os, src, dst)) {
    case COPY_FAILED:  error(c->log, rel, 0, "could not copy file"); break;
    case COPY_DONE:    c->copied++;  break;
    case COPY_SKIPPED: c->skipped++; break;
    }
}
