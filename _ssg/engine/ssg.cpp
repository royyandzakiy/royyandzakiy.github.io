// ssg: static site generator for royyandzakiy.com
//
// Unity build: a platform layer (platform/main.cpp, test/main_test.cpp)
// includes this file and implements the os_* API in core/os.cpp.
//
// This is free and unencumbered software released into the public domain.
#include "core/base.cpp"
#include "core/os.cpp"
#include "core/log.cpp"

#include "parser/highlight.cpp"
#include "parser/markdown.cpp"
#include "parser/frontmatter.cpp"
#include "engine/site.cpp"
#include "engine/newpost.cpp"
#include "engine/templates.cpp"


static std::string outpath(std::string_view url)  // "/x/y/" -> "x/y/index.html"
{
    return std::format("{}index.html", url.substr(1));
}

static void render(Ctx *c, Site *site)
{
    for (auto &p : site->posts) {
        Markdown md = markdown(p->body, p->src, p->bodyline, c->log);
        p->html    = std::move(md.html);
        p->excerpt = md.excerpt;
    }
    for (auto &p : site->pages) {
        p->html = markdown(p->body, p->src, p->bodyline, c->log).html;
    }
}

// Concatenate the stylesheet parts, formerly done with Liquid includes.
static std::string stylesheet(Ctx *c)
{
    static std::string_view parts[] = {"main.css", "print.css", "syntax.css", "dark.css"};
    std::string b;
    for (std::string_view part : parts) {
        std::string rel = join("css", part);
        std::optional<std::string> data = os_read(c->os, srcpath(c, rel));
        if (!data) {
            error(c->log, rel, 0, "could not read file");
        }
        b += data.value_or("");
        b += '\n';
    }
    return b;
}

static i32 build(Os *os, Options *opt, Log *log)
{
    i64 start = os_clock(os);
    Ctx c  = {};
    c.os   = os;
    c.opt  = opt;
    c.log  = log;

    Site site = {};
    site.now = opt->now;
    loadposts(&c, &site);
    walk(&c, &site, {}, relativeout(opt));
    if (log->errors) {
        return 1;
    }
    render(&c, &site);

    #define EMIT(path, call) do { \
            std::string b; \
            b.reserve(1<<16); \
            call; \
            emit(&c, path, b); \
        } while (0)

    for (auto &p : site.posts) {
        EMIT(outpath(p->url), postpage(&b, p.get()));
        EMIT(outpath(p->oldurl), redirectpage(&b, p.get()));
    }
    for (auto &t : site.tags) {
        std::string dir = std::format("tags/{}/", t->name);
        EMIT(dir + "index.html", tagpage(&b, t.get()));
        EMIT(dir + "feed/index.xml", tagfeed(&b, &site, t.get()));
    }
    for (auto &p : site.pages) {
        EMIT(p->out, aboutpage(&b, p.get()));
    }
    if (opt->excerpts) {
        EMIT("index.html",   homepage(&b, &site, std::ssize(site.posts)));
    } else {
        EMIT("index.html",   homelist(&b, &site));
    }
    EMIT("index/index.html", archivepage(&b, &site));
    EMIT("tags/index.html",  tagindexpage(&b, &site));
    EMIT("feed/index.xml",   atomfeed(&b, &site));
    EMIT("blog/index.rss",   rssfeed(&b, &site));
    emit(&c, "css/full.css", stylesheet(&c));
    #undef EMIT

    for (std::string &rel : site.statics) {
        copystatic(&c, rel);
    }

    if (!opt->quiet) {
        std::string b = std::format(
            "ssg: {} posts, {} pages written, {} files copied, {} unchanged in {} ms\n",
            site.posts.size(), c.written, c.copied, c.skipped, os_clock(os) - start);
        os_print(os, 2, b);
    }
    return log->errors ? 1 : 0;
}

// Debug: render a single Markdown file to standard output.
static i32 rendermd(Os *os, Options *opt, Log *log)
{
    std::optional<std::string> src = os_read(os, opt->mdfile);
    if (!src) {
        error(log, opt->mdfile, 0, "could not read file");
        return 1;
    }
    normalize(&*src);
    std::string_view body = *src;
    iz               line = 1;
    if (body.starts_with("---\n")) {
        FrontMatter fm = parsefrontmatter(body);
        if (!fm.ok) {
            error(log, opt->mdfile, fm.errline, fm.err);
            return 1;
        }
        body = fm.body;
        line = fm.bodyline;
    }
    os_print(os, 1, markdown(body, opt->mdfile, line, log).html);
    return 0;
}

// Summarize the source files a build reads, as walked by loadposts() and
// walk(): names, sizes, and modification times, order-independent.
static u64 fingerprint(Os *os, Options *opt, std::string_view rel, std::string_view outdir)
{
    std::string dir = rel.empty() ? std::string(opt->src) : join(opt->src, rel);
    u64 sum = 0;
    for (Dirent &e : os_list(os, dir)) {
        b32 posts = rel.empty() && e.isdir && e.name == "_posts";
        if (!posts && skipentry(rel, e.name)) {
            continue;
        }
        std::string child = join(rel, e.name);
        if (e.isdir) {
            if (child != outdir) {
                sum += fingerprint(os, opt, child, outdir);
            }
            continue;
        }
        u64 h = hash(child);
        h ^= (u64)e.size;
        h *= 1111111111111111111u;
        h ^= (u64)e.mtime;
        h *= 1111111111111111111u;
        sum += h ^ h>>32;
    }
    return sum;
}

// Rebuild whenever the source tree changes, until interrupted. Each build
// starts from nothing, so it is identical to a fresh build.
[[noreturn]] static void watch(Os *os, Options *opt)
{
    i32         interval = 250;  // ms
    std::string outdir   = relativeout(opt);
    u64         last     = fingerprint(os, opt, {}, outdir);
    for (;;) {
        Log log = {};
        if (!opt->fixedtime) {
            opt->now = os_now(os);
        }
        build(os, opt, &log);
        os_print(os, 2, log.buf);

        // Wait for a change, then for the tree to settle (e.g. mid-save)
        u64 fp = last;
        while (fp == last) {
            os_sleep(os, interval);
            fp = fingerprint(os, opt, {}, outdir);
        }
        for (u64 prev = last; fp != prev;) {
            prev = fp;
            os_sleep(os, interval);
            fp = fingerprint(os, opt, {}, outdir);
        }
        last = fp;
    }
}

static std::string_view usage =
"usage: ssg [-C SRCDIR] [-o OUTDIR] [-t UNIXTIME] [-w] [-n FILE] [-E] [-m FILE]\n"
"  -C DIR    source directory (default: .)\n"
"  -o DIR    output directory (default: _site)\n"
"  -t TIME   build time in Unix seconds (default: now)\n"
"  -w        watch: rebuild whenever the source changes\n"
"  -n FILE   new post: insert a draft into _posts, dated now (or -t);\n"
"            its first line, a \"# Title\" heading, becomes the title\n"
"  -E        debug: list every post and its excerpt on the home page\n"
"  -m FILE   debug: render one Markdown file (after front matter) to stdout\n"
"  -q        quiet: only print warnings and errors\n"
"  -h        print this message\n";

// The arguments, args[0] being the program name, must outlive the call.
static i32 ssg_main(Os *os, std::span<std::string_view const> args)
{
    Options opt = {};
    opt.src = ".";
    opt.out = "_site";
    for (uz i = 1; i < args.size(); i++) {
        std::string_view a = args[i];
        std::string_view v = i+1 < args.size() ? args[i+1] : std::string_view{};
        if (a == "-C" && v.data()) {
            opt.src = v; i++;
        } else if (a == "-o" && v.data()) {
            opt.out = v; i++;
        } else if (a == "-t" && v.data()) {
            opt.now = parseint(v);
            opt.fixedtime = 1;
            i++;
        } else if (a == "-m" && v.data()) {
            opt.mdfile = v; i++;
        } else if (a == "-n" && v.data()) {
            opt.newpost = v; i++;
        } else if (a == "-w") {
            opt.watch = 1;
        } else if (a == "-E") {
            opt.excerpts = 1;
        } else if (a == "-q") {
            opt.quiet = 1;
        } else if (a == "-h") {
            os_print(os, 1, usage);
            return 0;
        } else {
            os_print(os, 2, usage);
            return 1;
        }
    }
    if (!opt.fixedtime) {
        opt.now = os_now(os);
    }

    if (opt.watch && !opt.mdfile.data() && !opt.newpost.data()) {
        watch(os, &opt);
    }

    Log log = {};
    i32 status = 0;
    if (opt.mdfile.data()) {
        status = rendermd(os, &opt, &log);
    } else if (opt.newpost.data()) {
        status = newpost(os, &opt, &log);
    } else {
        status = build(os, &opt, &log);
    }

    os_print(os, 2, log.buf);
    if (log.errors && !status) {
        status = 1;
    }
    return status;
}
