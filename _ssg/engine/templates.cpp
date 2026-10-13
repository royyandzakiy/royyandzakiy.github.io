// Page templates: layouts and generated pages, printed into buffers
// This is free and unencumbered software released into the public domain.
//
// Every page reads as the output of a shell command: a prompt bar on top
// (it doubles as the menu), then "$ command" lines and their output.

static std::string_view site_url     = "https://royyandzakiy.com";
static std::string_view site_title   = "Royyan Dzakiy";
static std::string_view site_desc    = "Hobby computing blog";
static std::string_view site_tagline = "Zephyr, modern C++, ESP-IDF, and the build & test tooling around them.";
static std::string_view feed_uuid    = "060da7ea-d2f4-4736-b001-fb0e880dd589";
static iz               home_posts   = 10;  // rows in the home page listing

// The title is the concatenation of two parts. cwd is the directory shown
// in the prompt, such as "~/notes".
static void layouthead(std::string *b, std::string_view cwd,
                       std::string_view title, std::string_view title2 = {})
{
    *b += "<!DOCTYPE html>\n<html lang=\"en\">\n<title>";
    printhtml(b, title);
    printhtml(b, title2);
    *b += "</title>\n"
R"html(<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0"/>
<link rel="alternate" type="application/atom+xml" href="/feed/" title="Atom Feed"/>
<link rel="stylesheet" href="/css/full.css"/>
<script>try { var t = localStorage.getItem("theme"); if (t) document.documentElement.dataset.theme = t } catch (e) {}</script>

<header class="bar"><div class="col">
  <a class="prompt" href="/"><span class="p">royyan@rd</span>:<span class="d">)html";
    printhtml(b, cwd);
    *b +=
R"html(</span>$</a>
  <nav><a href="/">notes</a> <a href="/projects/">projects</a> <a href="/talks/">talks</a> <a href="/about/">about</a><span class="setting" hidden><span class="sep">|</span><button class="theme" type="button"></button></span></nav>
</div></header>
<script>
(function () {
    var root = document.documentElement, btn = document.querySelector(".theme");
    function dark() {
        return root.dataset.theme ? root.dataset.theme == "dark"
                                  : matchMedia("(prefers-color-scheme: dark)").matches;
    }
    function label() {
        btn.textContent = dark() ? "light" : "dark";
        btn.setAttribute("aria-label", "Switch to " + btn.textContent + " mode");
    }
    btn.onclick = function () {
        root.dataset.theme = dark() ? "light" : "dark";
        try { localStorage.setItem("theme", root.dataset.theme) } catch (e) {}
        label();
    };
    label();
    btn.parentNode.hidden = false;
})();
</script>

<main class="col">
)html";
}

static void layouttail(std::string *b)
{
    *b +=
R"(
<div class="cmd"><span class="p">$</span> <span class="cur"></span></div>
</main>

<footer class="col">
  <span class="c"># Public domain unless noted.</span>
  <a href="/feed/">feed</a> <a href="/tags/">tags</a> <a href="/uses/">uses</a>
  <a href="https://github.com/royyandzakiy" target="_blank" rel="noopener">github</a>
  <a href="https://www.linkedin.com/in/royyandzakiy/" target="_blank" rel="noopener">linkedin</a>
</footer>
</html>
)";
}

// A "$ command" line. The command is plain text.
static void cmdline(std::string *b, std::string_view cmd)
{
    *b += "<div class=\"cmd\"><span class=\"p\">$</span> ";
    printhtml(b, cmd);
    *b += "</div>\n";
}

// The last path segment of a post URL: "/blog/2026-04/x/" -> "x"
static std::string_view slugof(std::string_view url)
{
    url.remove_suffix(1);
    return url.substr(url.rfind('/') + 1);
}

// A size the way "ls -h" prints it, rounding up: 823, 1.8K, 12K.
static void printsize(std::string *b, iz n)
{
    iz tenths = (n*10 + 1023) / 1024;
    if (n < 1024) {
        printfmt(b, "{}", n);
    } else if (tenths < 100) {
        printfmt(b, "{}.{}K", tenths/10, tenths%10);
    } else {
        printfmt(b, "{}K", (n + 1023) / 1024);
    }
}

// One "ls -l" row. Permissions and size hide on narrow screens.
static void lsrow(std::string *b, Post *p)
{
    *b += "  <li><span class=\"pm\">-rw-r--r--</span><span class=\"pm sz\">";
    printsize(b, std::ssize(p->text));
    *b += "</span><time datetime=\"";
    printymd(b, p->time);
    *b += "\">";
    printymd(b, p->time);
    *b += "</time><a href=\"";
    printattr(b, p->url);
    *b += "\">";
    printhtml(b, p->title);
    *b += "</a></li>\n";
}

static void tagslinks(std::string *b, Post *p)
{
    for (iz i = 0; i < std::ssize(p->tags); i++) {
        if (i) *b += ", ";
        *b += "<a href=\"/tags/";
        printattr(b, p->tags[i]);
        *b += "/\">";
        printhtml(b, p->tags[i]);
        *b += "</a>";
    }
}

static void postpage(std::string *b, Post *p)
{
    std::string_view slug = slugof(p->url);
    layouthead(b, "~/notes", p->title);
    cmdline(b, std::format("cat {}.md", slug));

    *b += "<article class=\"post\">\n<pre class=\"fm\">---\ndate: ";
    printymd(b, p->time);
    *b += "\ntags: [";
    tagslinks(b, p);
    *b += "]\n---</pre>\n<h1>";
    printhtml(b, p->title);
    *b += "</h1>\n<div class=\"print-only url\">royyandzakiy.com";
    printhtml(b, p->url);
    *b += "</div>\n\n";
    *b += p->html;
    *b += "\n</article>\n\n";

    if (p->older || p->newer) {
        *b += "<nav class=\"pn\">\n";
        cmdline(b, std::format("ls notes/ | grep -C1 {}", slug));
        *b += "<div class=\"pnrow\">";
        if (p->older) {
            *b += "<a class=\"older\" href=\"";
            printattr(b, p->older->url);
            *b += "\">&larr; ";
            printhtml(b, p->older->title);
            *b += "</a>";
        }
        if (p->newer) {
            *b += "<a class=\"newer\" href=\"";
            printattr(b, p->newer->url);
            *b += "\">";
            printhtml(b, p->newer->title);
            *b += " &rarr;</a>";
        }
        *b += "</div>\n</nav>\n";
    }
    layouttail(b);
}

// Debug (-E): every post with its excerpt.
static void homepage(std::string *b, Site *site, iz limit)
{
    layouthead(b, "~", site_title);
    for (iz i = 0; i < std::ssize(site->posts) && i < limit; i++) {
        Post *p = site->posts[i].get();
        cmdline(b, std::format("head {}.md", slugof(p->url)));
        *b += "<article class=\"post\">\n<h1><a href=\"";
        printattr(b, p->url);
        *b += "\">";
        printhtml(b, p->title);
        *b += "</a></h1>\n";
        *b += p->html.substr(0, p->excerpt);
        *b += "\n</article>\n";
    }
    layouttail(b);
}

// The front page: whoami, the latest notes, and now.txt.
static void homelist(std::string *b, Site *site)
{
    layouthead(b, "~", site_title);
    cmdline(b, "whoami");
    *b += "<p class=\"out\">";
    printhtml(b, site_title);
    *b += " &mdash; embedded software engineer.<br><span class=\"c\"># ";
    printhtml(b, site_tagline);
    *b += "</span></p>\n";

    cmdline(b, "ls -l --sort=time notes/ | head");
    *b += "<ul class=\"ls\">\n";
    iz n = std::ssize(site->posts);
    for (iz i = 0; i < n && i < home_posts; i++) {
        lsrow(b, site->posts[i].get());
    }
    *b += "</ul>\n";
    if (n > home_posts) {
        printfmt(b, "<p class=\"out c\">... {} more &rarr; <a href=\"/index/\">ls notes/</a></p>\n",
                 n - home_posts);
    }

    if (!site->nowtext.empty()) {
        cmdline(b, "cat now.txt");
        *b += "<p class=\"out\">";
        printhtml(b, trim(site->nowtext));
        *b += "</p>\n";
    }
    layouttail(b);
}

// Every post: the full listing.
static void archivepage(std::string *b, Site *site)
{
    layouthead(b, "~", "All notes");
    cmdline(b, "ls -l notes/");
    printfmt(b, "<p class=\"out c\">total {}</p>\n<ul class=\"ls\">\n", site->posts.size());
    for (auto &p : site->posts) {
        lsrow(b, p.get());
    }
    *b += "</ul>\n";
    layouttail(b);
}

static void tagindexpage(std::string *b, Site *site)
{
    layouthead(b, "~", "Tags");
    cmdline(b, "ls tags/");
    *b += "<ul class=\"tagls\">\n";
    for (auto &t : site->tags) {
        *b += "  <li><a href=\"/tags/";
        printattr(b, t->name);
        *b += "/\">";
        printhtml(b, t->name);
        printfmt(b, "/</a> <span class=\"c\">{} notes &middot;</span> <a class=\"c\" href=\"/tags/", t->posts.size());
        printattr(b, t->name);
        *b += "/feed/\">feed</a></li>\n";
    }
    *b += "</ul>\n";
    layouttail(b);
}

static void tagpage(std::string *b, Tag *tag)
{
    layouthead(b, "~", "Notes tagged ", tag->name);
    cmdline(b, std::format("grep -l 'tags:.*{}' notes/*", tag->name));
    *b += "<ul class=\"ls\">\n";
    for (Post *p : tag->posts) {
        lsrow(b, p);
    }
    *b += "</ul>\n<p class=\"out c\"># Subscribe: <a href=\"/tags/";
    printattr(b, tag->name);
    *b += "/feed/\">";
    printhtml(b, tag->name);
    *b += " feed</a></p>\n";
    layouttail(b);
}

static void printcdata(std::string *b, std::string_view s)
{
    *b += "<![CDATA[";
    for (;;) {
        uz i = s.find("]]>");
        if (i == s.npos) {
            break;
        }
        *b += s.substr(0, i+2);
        *b += "]]><![CDATA[";
        s = s.substr(i+2);
    }
    *b += s;
    *b += "]]>";
}

static void atomentry(std::string *b, Post *p)
{
    *b += "  <entry>\n    <title>";
    printhtml(b, p->title);
    *b += "</title>\n    <link rel=\"alternate\" type=\"text/html\" href=\"";
    printattr(b, site_url);
    printattr(b, p->url);
    *b += "\"/>\n    <id>urn:uuid:";
    *b += p->uuid;
    *b += "</id>\n    <updated>";
    printiso(b, p->time);
    *b += "</updated>\n    ";
    for (iz i = 0; i < std::ssize(p->tags); i++) {
        *b += "<category term=\"";
        printattr(b, p->tags[i]);
        *b += "\"/>";
    }
    *b += "\n    <content type=\"html\">\n      ";
    printcdata(b, p->html);
    *b += "\n    </content>\n  </entry>\n";
}

static void atomauthor(std::string *b, std::string_view uri, std::string_view suffix)
{
    *b += "  <author>\n    <name>Royyan Dzakiy</name>\n    <uri>";
    *b += uri;
    *b += suffix;
    *b += "</uri>\n  </author>\n\n";
}

static void atomfeed(std::string *b, Site *site)
{
    *b += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
             "<feed xmlns=\"http://www.w3.org/2005/Atom\">\n\n  <title>";
    printhtml(b, site_title);
    *b += "</title>\n  <link rel=\"alternate\" type=\"text/html\" href=\"";
    *b += site_url;
    *b += "\"/>\n  <link rel=\"self\" type=\"application/atom+xml\" href=\"";
    *b += site_url;
    *b += "/feed/\"/>\n  <updated>";
    printiso(b, site->now);
    *b += "</updated>\n  <id>urn:uuid:";
    *b += feed_uuid;
    *b += "</id>\n\n";
    atomauthor(b, site_url, "/");
    for (iz i = 0; i < std::ssize(site->posts) && i < 8; i++) {
        atomentry(b, site->posts[i].get());
    }
    *b += "\n</feed>\n";
}

static void tagfeed(std::string *b, Site *site, Tag *tag)
{
    *b += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
             "<feed xmlns=\"http://www.w3.org/2005/Atom\">\n\n  <title>Articles tagged ";
    printhtml(b, tag->name);
    *b += " at ";
    printhtml(b, site_title);
    *b += "</title>\n  <link rel=\"alternate\" type=\"text/html\"\n        href=\"";
    *b += site_url;
    *b += "/tags/";
    printattr(b, tag->name);
    *b += "/\"/>\n  <link rel=\"self\" type=\"application/atom+xml\"\n        href=\"";
    *b += site_url;
    *b += "/tags/";
    printattr(b, tag->name);
    *b += "/feed/\"/>\n  <updated>";
    printiso(b, site->now);
    *b += "</updated>\n  <id>urn:uuid:";
    *b += tag->uuid;
    *b += "</id>\n\n";
    atomauthor(b, site_url, "");
    for (iz i = 0; i < std::ssize(tag->posts); i++) {
        atomentry(b, tag->posts[i]);
    }
    *b += "\n</feed>\n";
}

static void rssfeed(std::string *b, Site *site)
{
    *b += "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
             "<rss version=\"2.0\" xmlns:atom=\"http://www.w3.org/2005/Atom\">\n"
             "  <channel>\n    <title>";
    printhtml(b, site_title);
    *b += "</title>\n    <description>";
    printhtml(b, site_desc);
    *b += "</description>\n    <link>";
    *b += site_url;
    *b += "</link>\n    <atom:link href=\"";
    *b += site_url;
    *b += "/blog/index.rss\" rel=\"self\" type=\"application/rss+xml\"/>\n"
             "    <language>en-us</language>\n    <pubDate>";
    printrfc822(b, site->now);
    *b += "</pubDate>\n    <lastBuildDate>";
    printrfc822(b, site->now);
    *b += "</lastBuildDate>\n\n";
    for (iz i = 0; i < std::ssize(site->posts) && i < 8; i++) {
        Post *p = site->posts[i].get();
        *b += "    <item>\n      <title>";
        printhtml(b, p->title);
        *b += "</title>\n      <link>";
        *b += site_url;
        printhtml(b, p->url);
        *b += "</link>\n      <guid>";
        *b += site_url;
        printhtml(b, p->url);
        *b += "</guid>\n      <pubDate>";
        printrfc822(b, p->time);
        *b += "</pubDate>\n      <comments>";
        *b += site_url;
        printhtml(b, p->url);
        *b += "#disqus_thread</comments>\n      <description>\n        ";
        printcdata(b, p->html);
        *b += "\n      </description>\n    </item>\n";
    }
    *b += "\n  </channel>\n</rss>\n";
}

// A stub at a post's old permalink that forwards to its current URL.
static void redirectpage(std::string *b, Post *p)
{
    *b += "<!DOCTYPE html>\n<meta charset=\"utf-8\">\n<title>";
    printhtml(b, p->title);
    *b += "</title>\n<link rel=\"canonical\" href=\"";
    *b += site_url;
    printattr(b, p->url);
    *b += "\"/>\n<meta http-equiv=\"refresh\" content=\"0; url=";
    printattr(b, p->url);
    *b += "\"/>\n<p>Moved to <a href=\"";
    printattr(b, p->url);
    *b += "\">";
    printhtml(b, p->title);
    *b += "</a>.</p>\n";
}

// Markdown pages (about/, projects/, ...): "$ command" from the front
// matter, or "cat" of the file, then the page itself.
static void aboutpage(std::string *b, Page *p)
{
    layouthead(b, "~", p->title);
    cmdline(b, p->command.empty() ? std::format("cat {}", p->src) : std::string(p->command));
    *b += "<article class=\"page\">\n";
    *b += p->html;
    *b += "\n</article>\n";
    layouttail(b);
}

// 404.html, served by GitHub Pages for any missing path. The script
// shows the path actually requested; without it, a generic one.
static void notfoundpage(std::string *b)
{
    layouthead(b, "~", "No such file or directory");
    *b +=
R"(<div class="cmd"><span class="p">$</span> cd <span class="y nfpath">this/page</span></div>
<p class="out">bash: cd: <span class="nfpath">this/page</span>: <b>No such file or directory</b></p>
)";
    cmdline(b, "ls");
    *b +=
R"(<p class="out"><a href="/">notes/</a>  <a href="/projects/">projects/</a>  <a href="/talks/">talks/</a>  <a href="/about/">about</a></p>
<p class="out c"># It may have moved. Old links like /2025-12-17/concepts still redirect.</p>
<script>for (const e of document.querySelectorAll('.nfpath')) e.textContent = location.pathname</script>
)";
    layouttail(b);
}
