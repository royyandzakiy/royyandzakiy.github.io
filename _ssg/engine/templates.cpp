// Page templates: layouts and generated pages, printed into buffers
// This is free and unencumbered software released into the public domain.

static std::string_view site_url   = "https://royyandzakiy.com";
static std::string_view site_title = "Royyan Dzakiy";
static std::string_view site_desc  = "Hobby computing blog";
static std::string_view feed_uuid  = "060da7ea-d2f4-4736-b001-fb0e880dd589";

// The title is the concatenation of two parts.
static void layouthead(std::string *b, std::string_view title, std::string_view title2 = {})
{
    *b += "<!DOCTYPE html>\n<title>";
    printhtml(b, title);
    printhtml(b, title2);
    *b += "</title>\n"
R"(<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0"/>
<link rel="alternate" type="application/atom+xml" href="/feed/" title="Atom Feed"/>
<link rel="stylesheet" href="/css/full.css"/>

<main lang="en">
)";
}

static void layouttail(std::string *b)
{
    *b +=
R"(</main>

<header>
  <div class="container">
    <h1 class="site-title identity"><a href="/">Royyan Dzakiy</a></h1>
    <nav>
      <ul>
        <li class="nav index"><a href="/index/">Index</a></li>
        <li class="nav tags"><a href="/tags/">Tags</a></li>
        <li class="nav about"><a href="/about/">About</a></li>
        <li class="nav github"><a href="https://github.com/royyandzakiy">GitHub</a></li>
      </ul>
    </nav>
  </div>
</header>

<footer>
  <p>
    All information on this blog, unless otherwise noted, is
    hereby released into the public domain, with no rights
    reserved.
  </p>
</footer>
)";
}

// The date header shared by post pages and the front page.
static void postheader(std::string *b, Post *p)
{
    *b += "  <h2><a href=\"";
    printattr(b, p->url);
    *b += "\">";
    printhtml(b, p->title);
    *b += "</a></h2>\n  <time datetime=\"";
    printymd(b, p->time);
    *b += "\">\n    ";
    printlongdate(b, p->time);
    *b += "\n  </time>\n  <div class=\"print-only url\">\n    royyandzakiy.com";
    printhtml(b, p->url);
    *b += "\n  </div>\n\n";
}

static void posttags(std::string *b, Post *p)
{
    *b += "  <ul class=\"tags\">\n";
    for (iz i = 0; i < std::ssize(p->tags); i++) {
        *b += "    <li><a href=\"/tags/";
        printattr(b, p->tags[i]);
        *b += "/\">";
        printhtml(b, p->tags[i]);
        *b += "</a></li>\n";
    }
    *b += "  </ul>\n  <ol class=\"references print-only\"></ol>\n";
}

static void postpage(std::string *b, Post *p)
{
    layouthead(b, p->title);
    *b += "<article class=\"single\">\n";
    postheader(b, p);
    *b += p->html;
    *b += "\n\n";
    posttags(b, p);

    *b +=
R"(
  <nav class="no-print">
)";
    if (p->older) {
        *b += "    <div class=\"prev\">\n      <span class=\"marker\">«</span>\n      <a href=\"";
        printattr(b, p->older->url);
        *b += "\">\n        ";
        printhtml(b, p->older->title);
        *b += "\n      </a>\n    </div>\n";
    }
    if (p->newer) {
        *b += "    <div class=\"next\">\n      <span class=\"marker\">»</span>\n      <a href=\"";
        printattr(b, p->newer->url);
        *b += "\">\n        ";
        printhtml(b, p->newer->title);
        *b += "\n      </a>\n    </div>\n";
    }
    *b += "  </nav>\n</article>\n";
    layouttail(b);
}

static void homepage(std::string *b, Site *site, iz limit)
{
    layouthead(b, site_title);
    for (iz i = 0; i < std::ssize(site->posts) && i < limit; i++) {
        Post *p = site->posts[i];
        *b += "<article class=\"many\">\n";
        postheader(b, p);
        *b += "  <blockquote class=\"excerpt\" cite=\"";
        printattr(b, p->url);
        *b += "\">\n";
        *b += p->html.substr(0, p->excerpt);
        *b += "\n  </blockquote>\n  <div class=\"read-more\">[<a href=\"";
        printattr(b, p->url);
        *b += "\">…</a>]</div>\n\n";
        posttags(b, p);
        *b += "</article>\n";
    }
    layouttail(b);
}

static void postitem(std::string *b, Post *p)
{
    *b += "    <li>\n      <time datetime=\"";
    printymd(b, p->time);
    *b += "\">\n        ";
    printymd(b, p->time);
    *b += "\n      </time>\n      <a href=\"";
    printattr(b, p->url);
    *b += "\">";
    printhtml(b, p->title);
    *b += "</a>\n    </li>\n";
}

static void archivepage(std::string *b, Site *site)
{
    layouthead(b, "Archives");
    *b += "<article class=\"index\">\n  <h2>Archives</h2>\n  <p>\n"
             "    There are <span class=\"post-count\">";
    *b += std::to_string(std::ssize(site->posts));
    *b += "</span> articles.\n  </p>\n  <ul class=\"post-list\">\n";
    for (iz i = 0; i < std::ssize(site->posts); i++) {
        postitem(b, site->posts[i]);
    }
    *b += "  </ul>\n</article>\n";
    layouttail(b);
}

// The front page: every post, newest first.
static void homelist(std::string *b, Site *site)
{
    layouthead(b, site_title);
    *b += "<article class=\"index\">\n  <h2>Posts</h2>\n"
             "  <ul class=\"post-list\">\n";
    for (iz i = 0; i < std::ssize(site->posts); i++) {
        postitem(b, site->posts[i]);
    }
    *b += "  </ul>\n</article>\n";
    layouttail(b);
}

static void tagindexpage(std::string *b, Site *site)
{
    layouthead(b, "Tags");
    *b += "<article class=\"tag\">\n  <h2>Tags</h2>\n  <ul class=\"post-list\">\n";
    for (iz i = 0; i < std::ssize(site->tags); i++) {
        std::string_view name = site->tags[i]->name;
        *b += "    <li>\n      <a href=\"/tags/";
        printattr(b, name);
        *b += "/feed/\" class=\"feed\"></a>\n      <a href=\"/tags/";
        printattr(b, name);
        *b += "/\" class=\"tag-entry\">";
        printhtml(b, name);
        *b += "</a>\n    </li>\n";
    }
    *b += "  </ul>\n</article>\n";
    layouttail(b);
}

static void tagpage(std::string *b, Tag *tag)
{
    layouthead(b, "Posts tagged ", tag->name);
    *b += "<article class=\"tag\">\n  <h2>\n    <a class=\"feed\" href=\"/tags/";
    printattr(b, tag->name);
    *b += "/feed/\"></a>\n    Articles tagged \"";
    printhtml(b, tag->name);
    *b += "\"\n  </h2>\n  <ul class=\"post-list\">\n";
    for (iz i = 0; i < std::ssize(tag->posts); i++) {
        postitem(b, tag->posts[i]);
    }
    *b += "  </ul>\n</article>\n";
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
        atomentry(b, site->posts[i]);
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
        Post *p = site->posts[i];
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

static void aboutpage(std::string *b, Page *p)
{
    layouthead(b, p->title);
    *b += "<article class=\"single\">\n<h2>";
    printhtml(b, p->title);
    *b += "</h2>\n";
    *b += p->html;
    *b += "\n</article>\n";
    layouttail(b);
}
