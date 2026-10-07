// Syntax highlighting for fenced code blocks
// This is free and unencumbered software released into the public domain.
//
// Each block is lexed in one linear pass without allocation, and tokens
// are wrapped in <span class="X"> using a few classes shared by every
// language, styled by css/syntax.css:
//
//   c  comment              m  number
//   k  keyword              p  preprocessor, directive, decorator
//   t  type                 v  variable, sigil, register, symbol
//   s  string, character    f  definition name, label
//   gi gd gh  diff inserted, deleted, header
//
// Most languages share a table-driven lexer (lexgeneric) configured by
// an HlLang. Lisps, assembly, shell, make, diff, markup, and a handful of
// line-oriented formats have their own small lexers. None has to be
// exact, only nice: when in doubt, text is left plain.

enum {
    HL_PLAIN,
    HL_COMMENT,
    HL_KEYWORD,
    HL_TYPE,
    HL_STRING,
    HL_NUMBER,
    HL_PREPROC,
    HL_VAR,
    HL_FUNC,
    HL_INSERTED,
    HL_DELETED,
    HL_HEADING,
};

static std::string_view hlclasses[] = {"", "c", "k", "t", "s", "m", "p", "v", "f", "gi", "gd", "gh"};

struct Hl {
    std::string *b;
    std::string_view  s;     // the code
    iz   i;     // scan position
    iz   done;  // code before this position has been written
};

// Write the pending plain text before the scan position.
static void hlflush(Hl *x)
{
    printhtml(x->b, std::string_view(x->s.data()+x->done, x->s.data()+x->i));
    x->done = x->i;
}

static void hlopen(Hl *x, i32 cls)
{
    hlflush(x);
    *x->b += "<span class=\"";
    *x->b += hlclasses[cls];
    *x->b += "\">";
}

static void hlclose(Hl *x)
{
    hlflush(x);
    *x->b += "</span>";
}

// Emit the code from the scan position to end as one token.
static void hltoken(Hl *x, i32 cls, iz end)
{
    assert(end >= x->i && end <= std::ssize(x->s));
    if (cls!=HL_PLAIN && end>x->i) {
        hlopen(x, cls);
        x->i = end;
        hlclose(x);
    }
    x->i = end;
}


// Scanning helpers

static u8 hlat(std::string_view s, iz i)  // zero outside the string
{
    return i>=0 && i<std::ssize(s) ? (u8)s[i] : 0;
}

static b32 hlin(u8 c, std::string_view set)
{
    for (iz i = 0; i < std::ssize(set); i++) {
        if (c && c==(u8)set[i]) return 1;
    }
    return 0;
}

static b32 hlidchar(u8 c)
{
    return alnum(c) || c=='_' || c>=0x80;
}

static b32 hlidstart(u8 c)
{
    return letter(c) || c=='_' || c>=0x80;
}

static b32 hlhex(u8 c)
{
    return digit(c) || (lowercase(c)>='a' && lowercase(c)<='f');
}

static iz hlidend(std::string_view s, iz i)
{
    for (; i<std::ssize(s) && hlidchar((u8)s[i]); i++) {}
    return i;
}

static iz hleol(std::string_view s, iz i)
{
    for (; i<std::ssize(s) && s[i]!='\n'; i++) {}
    return i;
}

static iz hlblank(std::string_view s, iz i)  // skip spaces and tabs
{
    for (; i<std::ssize(s) && (s[i]==' ' || s[i]=='\t'); i++) {}
    return i;
}

static b32 hlhas(std::string_view s, iz i, std::string_view pat)
{
    return std::ssize(pat) && i+std::ssize(pat)<=std::ssize(s) && (s.substr(i)).substr(0, std::ssize(pat))==pat;
}

// End of the UTF-8 sequence starting at s[i].
static iz hlcharend(std::string_view s, iz i)
{
    u8 c = s[i];
    iz n = c>=0xf0 ? 4 : c>=0xe0 ? 3 : c>=0xc0 ? 2 : 1;
    return i+n<std::ssize(s) ? i+n : std::ssize(s);
}

// End of a number starting with a digit, or a period and digit:
// decimal, hex, floats with exponents, and any alphanumeric suffix.
static iz hlnumber(std::string_view s, iz i)
{
    b32 hex = s[i]=='0' && lowercase(hlat(s, i+1))=='x';
    iz  j   = i;
    for (; j < std::ssize(s); j++) {
        u8 c = s[j];
        if (alnum(c) || c=='_') {
            continue;
        } else if (c=='.' && digit(hlat(s, j+1))) {
            continue;
        } else if ((c=='+' || c=='-') && j>i) {
            u8 e = lowercase((u8)s[j-1]);
            if (hex ? e=='p' : e=='e') continue;
        }
        break;
    }
    return j;
}

// A sorted word table
using HlWords = std::span<std::string_view const>;

static b32 hlmember(HlWords w, std::string_view s)
{
    return std::ranges::binary_search(w, s);
}

// Like hlmember, but optionally ignoring ASCII case (lowercase table).
static b32 hlword(HlWords w, std::string_view s, b32 fold)
{
    char tmp[32];
    if (!fold) {
        return hlmember(w, s);
    } else if (std::ssize(s) > std::ssize(tmp)) {
        return 0;
    }
    for (iz i = 0; i < std::ssize(s); i++) {
        tmp[i] = (char)lowercase(s[i]);
    }
    return hlmember(w, {tmp, s.size()});
}

#include "langs.cpp"


// Language configuration

enum {
    LEX_CPP       = 1<<0,   // # preprocessor lines
    LEX_FUNCDEF   = 1<<1,   // "type name(" at file scope defines name
    LEX_TSUFFIX   = 1<<2,   // names ending in _t are types
    LEX_CAMEL     = 1<<3,   // capitalized names are types
    LEX_REGEX     = 1<<4,   // /regex/ literals where an operand may start
    LEX_TRIPLE    = 1<<5,   // """triple-quoted""" strings
    LEX_TRANSPOSE = 1<<6,   // ' after an operand is an operator
    LEX_CASEFOLD  = 1<<7,   // keywords ignore case
    LEX_DOUBLING  = 1<<8,   // a doubled quote is a literal quote
    LEX_DOLLAR    = 1<<9,   // $name variables
    LEX_PERL      = 1<<10,  // @array, %hash, $$ref, $#array
    LEX_AT        = 1<<11,  // @decorators and @annotations
    LEX_SYMBOL    = 1<<12,  // :symbols
    LEX_BASIC     = 1<<13,  // name$ sigils, &H hex, REM comments
    LEX_VIM       = 1<<14,  // " comments, <key> notation
    LEX_KEYS      = 1<<15,  // "string": is a key
    LEX_IDDOLLAR  = 1<<16,  // $ is a name character
    LEX_ELISP     = 1<<17,  // ?c characters
    LEX_WAT       = 1<<18,  // WebAssembly text format
    LEX_NASM      = 1<<19,
    LEX_ATT       = 1<<20,
    LEX_ARM       = 1<<21,
    LEX_CLASSES   = 1<<22,  // a name declared after a tdef is a type
    LEX_METHODS   = 1<<23,  // with FUNCDEF: at any brace depth
    LEX_OUTPUTS   = 1<<24,  // "function out = name(": def after the "="
};

struct HlLang {
    void   (*lex)(Hl *, HlLang *);
    std::string_view     comment;    // line comments
    std::string_view     comment2;
    std::string_view     open;       // block comments
    std::string_view     close;
    std::string_view     quotes;     // string delimiters
    u8      escape;     // escape character in strings, or zero
    i32     flags;
    HlWords keywords;
    HlWords types;
    HlWords defs;       // keywords after which a name is being defined
    HlWords tdefs;      // keywords after which a name is a type
    HlWords prefixes;   // string literal prefixes, like L"..."
};


// The table-driven lexer for C-like and other conventional languages

// End of the string literal starting at s[i]. Only backquoted strings
// may span lines unless the newline is escaped.
static iz hlstring(std::string_view s, iz i, HlLang *l)
{
    u8  q     = s[i];
    u8  esc   = l->escape;
    b32 multi = q == '`';
    if ((l->flags & LEX_TRIPLE) && hlat(s, i+1)==q && hlat(s, i+2)==q) {
        for (iz j = i+3; j < std::ssize(s); j++) {
            if (esc && (u8)s[j]==esc) {
                j++;
            } else if ((u8)s[j]==q && hlat(s, j+1)==q && hlat(s, j+2)==q) {
                return j + 3;
            }
        }
        return std::ssize(s);
    }
    for (iz j = i+1; j < std::ssize(s); j++) {
        u8 c = s[j];
        if (esc && c==esc) {
            j++;
        } else if (c == q) {
            if (!(l->flags & LEX_DOUBLING) || hlat(s, j+1)!=q) {
                return j + 1;
            }
            j++;
        } else if (c=='\n' && !multi) {
            return j;
        }
    }
    return std::ssize(s);
}

// Capitalized with a lowercase letter, like Arena or Lisp_Object, but not
// a prefixed constant like SYS_write.
static b32 hlcamel(std::string_view w)
{
    if (w.empty() || w[0]<'A' || w[0]>'Z') {
        return 0;
    }
    iz  i     = 1;
    b32 lower = 0;
    for (; i<std::ssize(w) && w[i]!='_'; i++) {
        lower |= w[i]>='a' && w[i]<='z';
    }
    if (!lower && i+1<std::ssize(w) && w[i+1]>='a' && w[i+1]<='z') {
        return 0;
    }
    for (; i < std::ssize(w); i++) {
        lower |= w[i]>='a' && w[i]<='z';
    }
    return lower;
}

// End of the regular expression literal at s[i], or zero if none.
static iz hlregex(std::string_view s, iz i)
{
    b32 inclass = 0;
    for (iz j = i+1; j < std::ssize(s); j++) {
        u8 c = s[j];
        if (c == '\n') {
            return 0;
        } else if (c == '\\') {
            j++;
        } else if (c == '[') {
            inclass = 1;
        } else if (c == ']') {
            inclass = 0;
        } else if (c=='/' && !inclass) {
            for (j++; j<std::ssize(s) && letter((u8)s[j]); j++) {}  // flags
            return j;
        }
    }
    return 0;
}

static void lexgeneric(Hl *x, HlLang *l)
{
    std::string_view s     = x->s;
    i32 flags = l->flags;
    b32 fold  = flags & LEX_CASEFOLD;
    b32 bol   = 1;         // nothing but whitespace so far on this line
    b32 value = 0;         // last token was an operand
    b32 pp    = 0;         // inside a preprocessor directive
    i32 depth = 0;         // brace depth
    i32 paren = 0;         // parenthesis depth
    b32 prev  = 0;         // last token was a name, or "name *"
    i32 next  = HL_PLAIN;  // class for a name following a def or tdef
    i32 tdef  = -1;        // brace depth of a typedef in progress
    iz  defat = -1;        // position of a name after "function out ="
    std::string_view known[16];         // type names declared so far
    i32 nknown = 0;

    while (x->i < std::ssize(s)) {
        iz i = x->i;
        u8 c = s[i];

        if (c == '\n') {
            if (pp && hlat(s, i-1)!='\\') {
                pp   = 0;
                prev = 0;
            }
            bol  = 1;
            next = HL_PLAIN;
            prev &= !whitespace(hlat(s, i+1));  // "type\nname(" only unindented
            x->i++;
            continue;
        } else if (whitespace(c)) {
            x->i++;
            continue;
        }
        b32 first = bol;
        i32 want  = next;
        bol  = 0;
        next = HL_PLAIN;

        if (hlhas(s, i, l->open)) {
            uz end = s.find(l->close, i+l->open.size());
            hltoken(x, HL_COMMENT, end==s.npos ? std::ssize(s) : (iz)(end+l->close.size()));
            continue;
        }

        if (hlhas(s, i, l->comment) || hlhas(s, i, l->comment2) ||
                ((flags & LEX_VIM) && c=='"' && first)) {
            hltoken(x, HL_COMMENT, hleol(s, i));
            continue;
        }

        if ((flags & LEX_CPP) && c=='#' && first) {
            iz  beg = hlblank(s, i+1);
            iz  end = hlidend(s, beg);
            std::string_view dir = std::string_view(s.data()+beg, s.data()+end);
            hltoken(x, HL_PREPROC, end);
            pp   = 1;
            prev = 0;
            if (dir=="include" || dir=="import") {
                iz j = hlblank(s, end);
                iz k = j;
                for (; k<std::ssize(s) && s[k]!='>' && s[k]!='\n'; k++) {}
                if (hlat(s, j)=='<' && hlat(s, k)=='>') {
                    x->i = j;
                    hltoken(x, HL_STRING, k+1);
                }
            } else if (dir == "define") {
                next = HL_FUNC;
            }
            continue;
        }

        if ((flags & LEX_AT) && c=='@' && hlidstart(hlat(s, i+1))) {
            iz end = hlidend(s, i+1);
            if (!hlword(l->keywords, std::string_view(s.data()+i+1, s.data()+end), fold)) {
                while (hlat(s, end)=='.' && hlidstart(hlat(s, end+1))) {
                    end = hlidend(s, end+1);
                }
                hltoken(x, HL_PREPROC, end);
                value = prev = 0;
                continue;
            }
        }

        b32 perl = flags & LEX_PERL;
        if (((flags & LEX_DOLLAR) && c=='$') ||
                (perl && (c=='@' || (c=='%' && !value)))) {
            iz j = i + 1;
            if (perl && c=='$') {
                for (; hlat(s, j)=='$'; j++) {}
                j += hlat(s, j)=='#';
            }
            iz end = hlidend(s, j);
            if (perl && c=='$' && end==j && hlin(hlat(s, j), "/\\,;.!@&")) {
                end = j + 1;
            }
            if (end > j) {
                hltoken(x, HL_VAR, end);
                value = prev = 1;
                continue;
            }
        }

        if ((flags & LEX_SYMBOL) && c==':' && hlidstart(hlat(s, i+1)) &&
                !hlidchar(hlat(s, i-1)) && hlat(s, i-1)!=':') {
            hltoken(x, HL_VAR, hlidend(s, i+1));
            value = 1;
            continue;
        }

        if ((flags & LEX_VIM) && c=='<' && letter(hlat(s, i+1))) {
            iz end = i + 1;
            for (; end<std::ssize(s) && (alnum((u8)s[end]) || s[end]=='-'); end++) {}
            if (hlat(s, end) == '>') {
                hltoken(x, HL_VAR, end+1);
                continue;
            }
        }

        if ((flags & LEX_TRANSPOSE) && c=='\'' &&
                (hlidchar(hlat(s, i-1)) || hlin(hlat(s, i-1), ")]}.'"))) {
            x->i++;
            value = 1;
            continue;
        }

        if (hlin(c, l->quotes)) {
            iz  end = hlstring(s, i, l);
            b32 key = (flags & LEX_KEYS) && hlat(s, hlblank(s, end))==':';
            hltoken(x, key ? HL_VAR : HL_STRING, end);
            value = 1;
            prev  = 0;
            continue;
        }

        if (digit(c) || (c=='.' && digit(hlat(s, i+1)) && hlat(s, i-1)!='.')) {
            hltoken(x, HL_NUMBER, hlnumber(s, i));
            value = 1;
            prev  = 0;
            continue;
        }

        if ((flags & LEX_BASIC) && c=='&' && lowercase(hlat(s, i+1))=='h' &&
                hlhex(hlat(s, i+2))) {
            iz end = i + 2;
            for (; end<std::ssize(s) && hlhex((u8)s[end]); end++) {}
            hltoken(x, HL_NUMBER, end + (hlat(s, end)=='&'));
            value = 1;
            continue;
        }

        if (hlidstart(c) || ((flags & LEX_IDDOLLAR) && c=='$')) {
            iz end = i;
            for (; end < std::ssize(s); end++) {
                u8  d    = s[end];
                b32 more = (d=='$' && (flags & LEX_IDDOLLAR)) ||
                           (d=='-' && (flags & LEX_VIM));  // utf-8
                if (!hlidchar(d) && !more) break;
            }
            if ((flags & LEX_BASIC) && hlin(hlat(s, end), "$%&!#")) {
                end++;
            }
            std::string_view word = std::string_view(s.data()+i, s.data()+end);

            if (hlin(hlat(s, end), l->quotes) && hlmember(l->prefixes, word)) {
                hltoken(x, HL_STRING, hlstring(s, end, l));
                value = 1;
                prev  = 0;
                continue;
            }

            if ((flags & LEX_BASIC) && hlword(HlWords(basicrem), word, 1)) {
                hltoken(x, HL_COMMENT, hleol(s, i));
                continue;
            }

            // A name after a period is a member, never a keyword
            b32 member  = hlat(s, i-1)=='.' && hlat(s, i-2)!='.';
            b32 keyword = !member && hlword(l->keywords, word, fold);
            iz  open    = hlblank(s, end);
            u8  after   = hlat(s, open);
            b32 call    = after == '(';
            b32 isknown = 0;
            for (i32 k = 0; k<nknown && !isknown; k++) {
                isknown = known[k] == word;
            }
            want = i==defat ? HL_FUNC : want;
            i32 cls = HL_PLAIN;
            if (keyword) {
                cls = HL_KEYWORD;
                if (hlword(l->defs, word, fold)) {
                    next = HL_FUNC;
                    if ((flags & LEX_OUTPUTS) && first) {
                        // function [a, b] = name(...)
                        iz eq = end;
                        for (; eq<std::ssize(s) && !hlin((u8)s[eq], "=(\n"); eq++) {}
                        if (hlat(s, eq) == '=') {
                            next  = HL_PLAIN;
                            defat = hlblank(s, eq+1);
                        }
                    }
                } else if (hlword(l->tdefs, word, fold)) {
                    next = HL_TYPE;
                }
            } else if (want != HL_PLAIN) {
                cls = want;
            } else if (member) {
                // plain
            } else if (isknown || hlword(l->types, word, fold)) {
                cls = HL_TYPE;
            } else if ((flags & LEX_TSUFFIX) && std::ssize(word)>2 && word.ends_with("_t")) {
                cls = HL_TYPE;
            } else if ((flags & LEX_CAMEL) && !call && hlcamel(word)) {
                cls = HL_TYPE;
            }
            // "type name(" declares a function at file scope (or in a
            // class, with METHODS), but not inside parentheses, nor when
            // the parenthesis holds an expression: C++ "Guard g(&lock)"
            u8  arg    = hlat(s, hlblank(s, open+1));
            b32 params = !digit(arg) && !hlin(arg, "&*!-\"'");
            b32 scope  = !depth || (flags & LEX_METHODS);
            if (cls==HL_PLAIN && (flags & LEX_FUNCDEF) && call && prev &&
                    params && scope && !paren && !pp && !member) {
                cls = HL_FUNC;
            }

            // Remember declared types: typedef names ("typedef ... T;"),
            // and in C++ also class names and template parameters.
            if (keyword && word=="typedef") {
                tdef = depth;
            }
            b32 declared = (want==HL_TYPE && (flags & LEX_CLASSES)) ||
                           (cls==HL_PLAIN && tdef==depth && !paren && hlin(after, ";,"));
            if (declared && !keyword && !isknown && nknown<std::ssize(known)) {
                known[nknown++] = word;
            }
            cls = declared && !keyword ? HL_TYPE : cls;
            hltoken(x, cls, end);
            value = !keyword || hlmember(HlWords(valuekeywords), word);
            prev  = !keyword;
            continue;
        }

        // Operators and punctuation
        if (c=='/' && (flags & LEX_REGEX) && !value) {
            iz end = hlregex(s, i);
            if (end) {
                hltoken(x, HL_STRING, end);
                value = 1;
                prev  = 0;
                continue;
            }
        }
        if (!pp && c=='{') {
            depth++;
        } else if (!pp && c=='}') {
            depth -= depth > 0;
        } else if (c==';' && tdef==depth) {
            tdef = -1;
        } else if (!pp && c=='(') {
            paren++;
        } else if (!pp && c==')') {
            paren -= paren > 0;
        } else if (c==':' && hlat(s, i+1)==':') {
            x->i += 2;  // scope operator: keep prev for Class::method(
            value = 0;
            continue;
        }
        // Keep prev through a pointer declarator, but not multiplication
        // (spaced both sides), and through the end of template arguments
        // as in "span<T> f(", but not a comparison (space before).
        b32 lspace = hlin(hlat(s, i-1), " \t");
        b32 rspace = hlin(hlat(s, i+1), " \t");
        b32 ptr    = c=='*' && !(lspace && rspace);
        b32 targs  = c=='>' && !lspace && (rspace || hlat(s, i+1)=='>');
        prev  = prev && (ptr || targs);
        value = c==')' || c==']';
        x->i++;
    }
}


// Lisp family: Emacs Lisp, Common Lisp, Scheme, Clojure, WebAssembly text

static b32 hllispdelim(u8 c)
{
    return whitespace(c) || hlin(c, "()[]{}\";'`,");
}

static iz hllispsym(std::string_view s, iz i)  // end of the symbol at s[i]
{
    for (; i<std::ssize(s) && !hllispdelim((u8)s[i]); i++) {
        i += s[i] == '\\';  // escaped character
    }
    return i<std::ssize(s) ? i : std::ssize(s);
}

static iz hllispstr(std::string_view s, iz i)  // end of the string at s[i]
{
    for (iz j = i+1; j < std::ssize(s); j++) {
        if (s[j] == '\\') {
            j++;
        } else if (s[j] == '"') {
            return j + 1;
        }
    }
    return std::ssize(s);
}

static b32 hllispnum(std::string_view w)
{
    iz i = 0;
    iz n = 0;  // digits
    i += std::ssize(w) && (w[0]=='+' || w[0]=='-');
    for (; i<std::ssize(w) && digit(w[i]); i++, n++) {}
    if (i<std::ssize(w) && w[i]=='.') {
        for (i++; i<std::ssize(w) && digit(w[i]); i++, n++) {}
    }
    if (n && i<std::ssize(w) && lowercase(w[i])=='e') {
        i++;
        i += i<std::ssize(w) && (w[i]=='+' || w[i]=='-');
        iz e = i;
        for (; i<std::ssize(w) && digit(w[i]); i++) {}
        n *= i > e;
    } else if (n && i<std::ssize(w) && w[i]=='/') {  // ratio
        iz d = ++i;
        for (; i<std::ssize(w) && digit(w[i]); i++) {}
        n *= i > d;
    }
    return n && i==std::ssize(w);
}

static void lexlisp(Hl *x, HlLang *l)
{
    std::string_view s    = x->s;
    b32 wat  = l->flags & LEX_WAT;
    b32 head = 0;         // next symbol is the head of a form
    i32 next = HL_PLAIN;  // class for the next symbol

    while (x->i < std::ssize(s)) {
        iz i = x->i;
        u8 c = s[i];
        if (whitespace(c)) {
            x->i++;
            continue;
        }
        b32 ishead = head;
        i32 want   = next;
        head = 0;
        next = HL_PLAIN;

        if (c == ';') {
            hltoken(x, HL_COMMENT, hleol(s, i));

        } else if (wat && c=='(' && hlat(s, i+1)==';') {  // (; nesting ;)
            iz  j    = i;
            i32 nest = 0;
            while (j < std::ssize(s)) {
                if (s[j]=='(' && hlat(s, j+1)==';') {
                    nest++;
                    j += 2;
                } else if (s[j]==';' && hlat(s, j+1)==')') {
                    j += 2;
                    if (!--nest) break;
                } else {
                    j++;
                }
            }
            hltoken(x, HL_COMMENT, j);

        } else if (c == '(') {
            head = 1;
            next = want==HL_FUNC ? HL_FUNC : HL_PLAIN;  // (define (name ...
            x->i++;

        } else if (c == '"') {
            hltoken(x, HL_STRING, hllispstr(s, i));

        } else if (c=='\'' || c=='`' || c==',') {
            iz j = i + 1 + (c==',' && hlat(s, i+1)=='@');
            iz end = hllispsym(s, j);
            if (c=='\'' && end>j) {
                hltoken(x, HL_VAR, end);  // 'symbol
            } else {
                x->i = j;
            }

        } else if (c == '#') {
            u8  d   = hlat(s, i+1);
            iz  end = hllispsym(s, i+1);
            std::string_view w   = std::string_view(s.data()+i+1, s.data()+end);
            if (d=='\'' || d==':') {  // #'function #:uninterned
                end = hllispsym(s, i+2);
                hltoken(x, end>i+2 ? HL_VAR : HL_PLAIN, end);
            } else if (d == '\\') {  // #\c
                end = i+2<std::ssize(s) ? hllispsym(s, hlcharend(s, i+2)) : std::ssize(s);
                hltoken(x, HL_STRING, end);
            } else if (d == '"') {  // #"regex"
                hltoken(x, HL_STRING, hllispstr(s, i+1));
            } else if (w=="t" || w=="f" || w=="true" || w=="false") {
                hltoken(x, HL_KEYWORD, end);
            } else if (std::ssize(w)>1 && hlin(lowercase(d), "box")) {
                b32 ok = 1;
                for (iz k = 1; k < std::ssize(w); k++) {
                    ok &= hlhex(w[k]) || (k==1 && (w[k]=='-' || w[k]=='+'));
                }
                hltoken(x, ok ? HL_NUMBER : HL_PLAIN, ok ? end : i+1);
            } else {
                x->i++;
            }

        } else if ((l->flags & LEX_ELISP) && c=='?' && i+1<std::ssize(s)) {  // ?c
            iz j = i + 1;
            j += s[j]=='\\' && j+1<std::ssize(s);
            hltoken(x, HL_STRING, hlcharend(s, j));

        } else if (hlin(c, ")[]{}")) {
            x->i++;

        } else {
            iz  end = hllispsym(s, i);
            end += end == i;  // lone backslash
            std::string_view w   = std::string_view(s.data()+i, s.data()+end);
            i32 cls = HL_PLAIN;
            if (hllispnum(w)) {
                cls = HL_NUMBER;
            } else if (c == ':') {
                cls = HL_VAR;  // :keyword
            } else if (wat && c=='$') {
                cls = want==HL_FUNC ? HL_FUNC : HL_VAR;
            } else if (ishead && hlmember(wat ? HlWords(watdefs) : HlWords(lispdefs), w)) {
                cls  = HL_KEYWORD;
                next = HL_FUNC;
            } else if (ishead && (wat || hlmember(HlWords(lispheads), w))) {
                cls = HL_KEYWORD;
            } else if (want == HL_FUNC) {
                cls = HL_FUNC;
            } else if (wat) {
                cls = hlmember(HlWords(wattypes), w) ? HL_TYPE : HL_PLAIN;
            } else if (w=="t" || w=="nil" || w=="true" || w=="false") {
                cls = HL_KEYWORD;
            } else if (c=='&' && std::ssize(w)>1) {
                cls = HL_KEYWORD;  // &optional
            }
            hltoken(x, cls, end);
        }
    }
}


// Assembly: NASM (Intel), GNU as (AT&T), AArch64

static b32 hlasmchar(u8 c)
{
    return hlidchar(c) || c=='.' || c=='@' || c=='?';
}

static b32 hlx86reg(std::string_view w)  // lowercase
{
    static std::string_view numbered[] = {"xmm", "ymm", "zmm", "mm", "st", "cr", "dr", "k", "r"};
    if (hlmember(HlWords(x86regs), w)) {
        return 1;
    }
    for (iz p = 0; p < std::ssize(numbered); p++) {
        if (!w.starts_with(numbered[p])) {
            continue;
        }
        std::string_view rest = w.substr(std::ssize(numbered[p]));
        iz  n    = 0;
        for (; n<std::ssize(rest) && n<3 && digit(rest[n]); n++) {}
        rest = rest.substr(n);
        b32 size = numbered[p]=="r" && std::ssize(rest)==1 && hlin(rest[0], "bwdl");  // r8d
        if (n && n<3 && (rest.empty() || size)) {
            return 1;
        }
    }
    return 0;
}

static b32 hla64reg(std::string_view w)  // lowercase
{
    if (hlmember(HlWords(a64regs), w)) {
        return 1;
    } else if (std::ssize(w)<2 || std::ssize(w)>3 || !hlin(w[0], "xwvqdshb")) {
        return 0;
    }
    for (iz i = 1; i < std::ssize(w); i++) {
        if (!digit(w[i])) return 0;
    }
    return 1;
}

static void lexasm(Hl *x, HlLang *l)
{
    std::string_view s    = x->s;
    b32 nasm = l->flags & LEX_NASM;
    b32 att  = l->flags & LEX_ATT;
    b32 arm  = l->flags & LEX_ARM;
    b32 stmt = 1;         // expecting a label or mnemonic
    i32 next = HL_PLAIN;  // class for the next name

    while (x->i < std::ssize(s)) {
        iz i = x->i;
        u8 c = s[i];
        if (c == '\n') {
            stmt = hlat(s, i-1) != '\\';
            next = HL_PLAIN;
            x->i++;
            continue;
        } else if (whitespace(c)) {
            x->i++;
            continue;
        }
        b32 start = stmt;
        i32 want  = next;
        stmt = 0;
        next = HL_PLAIN;

        b32 comment = att ? c=='#' : c==';';
        comment |= !nasm && c=='/' && hlat(s, i+1)=='/';
        if (comment) {
            hltoken(x, HL_COMMENT, hleol(s, i));

        } else if (c=='/' && hlat(s, i+1)=='*' && !nasm) {
            uz end = s.find("*/", i+2);
            hltoken(x, HL_COMMENT, end==s.npos ? std::ssize(s) : (iz)end+2);

        } else if (c == ';') {  // GNU as statement separator
            stmt = 1;
            x->i++;

        } else if (c=='\'' && !nasm) {  // GNU as character: 'c or 'c'
            iz end = i + 1 + (hlat(s, i+1)=='\\');
            end = end<std::ssize(s) && s[end]!='\n' ? hlcharend(s, end) : end;
            hltoken(x, HL_STRING, end + (hlat(s, end)=='\''));

        } else if (c=='"' || c=='\'' || (c=='`' && nasm)) {
            iz end = i + 1;
            for (; end<std::ssize(s) && (u8)s[end]!=c && s[end]!='\n'; end++) {
                end += s[end]=='\\' && end+1<std::ssize(s) && s[end+1]!='\n';
            }
            hltoken(x, HL_STRING, end + (hlat(s, end)==c));

        } else if (c=='%' && hlidstart(hlat(s, i+1)) && (att || start)) {
            iz  end = hlidend(s, i+1);
            std::string_view dir = std::string_view(s.data()+i+1, s.data()+end);
            b32 def = dir=="define" || dir=="xdefine" || dir=="assign" || dir=="macro";
            if (nasm && def) {
                next = HL_FUNC;
            }
            hltoken(x, att ? HL_VAR : HL_PREPROC, end);  // %reg or %directive

        } else if (((att && c=='$') || (arm && c=='#')) &&
                (digit(hlat(s, i+1)) || (hlat(s, i+1)=='-' && digit(hlat(s, i+2))))) {
            iz j = i + 1 + (hlat(s, i+1)=='-');
            hltoken(x, HL_NUMBER, hlnumber(s, j));  // immediate

        } else if (digit(c)) {
            iz  end = hlnumber(s, i);
            iz  n   = i;
            for (; n<end && digit((u8)s[n]); n++) {}
            b32 label = !nasm && start && hlat(s, end)==':' && n==end;
            b32 ref   = !nasm && n==end-1 && hlin((u8)s[n], "bf");  // 1f, 0b
            hltoken(x, label||ref ? HL_FUNC : HL_NUMBER, end);
            if (label) {
                x->i++;  // colon
                stmt = 1;
            }

        } else if (hlasmchar(c) && !digit(c)) {
            iz end = i;
            for (; end < std::ssize(s); end++) {
                u8 d = s[end];
                if (!hlasmchar(d) && !(nasm && d=='$')) break;
            }
            std::string_view w = std::string_view(s.data()+i, s.data()+end);
            char tmp[32];
            std::string_view lw = {tmp, w.size()<sizeof(tmp) ? w.size() : 0};
            for (iz k = 0; k < std::ssize(lw); k++) {
                tmp[k] = (char)lowercase(w[k]);
            }

            i32 cls = HL_PLAIN;
            if (start && hlat(s, end)==':') {
                hltoken(x, HL_FUNC, end);
                x->i++;  // colon
                stmt = 1;
                continue;
            } else if (start) {
                cls  = HL_KEYWORD;
                stmt = hlmember(HlWords(asmprefixes), lw);
                if ((!nasm && c=='.') || (nasm && hlmember(HlWords(nasmdirectives), lw))) {
                    cls = HL_PREPROC;
                }
            } else if (want == HL_FUNC) {
                cls = HL_FUNC;
            } else if (nasm ? hlx86reg(lw) : (arm && hla64reg(lw))) {
                cls = HL_VAR;
            } else if (nasm && hlmember(HlWords(nasmsizes), lw)) {
                cls = HL_TYPE;
            }
            hltoken(x, cls, end);

        } else {
            x->i++;
        }
    }
}


// Shell: sh and bash

enum { HL_SHDEPTH = 8 };  // limit on nested $( ) substitutions

static b32 hlshword(u8 c)
{
    return !whitespace(c) && !hlin(c, "'\"`$;|&()<>\\");
}

static void hlshcode(Hl *x, i32 depth, b32 nested);

// A $ expansion at the scan position.
static void hlshdollar(Hl *x, i32 depth)
{
    std::string_view s = x->s;
    iz  i = x->i;
    u8  d = hlat(s, i+1);
    if (d == '{') {
        iz  end  = i + 1;
        i32 nest = 0;
        for (; end < std::ssize(s); end++) {
            nest += s[end]=='{';
            if (s[end]=='}' && !--nest) {
                end++;
                break;
            }
        }
        hltoken(x, HL_VAR, end<std::ssize(s) ? end : std::ssize(s));
    } else if (d=='(' && depth<HL_SHDEPTH) {
        hltoken(x, HL_VAR, i+2);
        hlshcode(x, depth+1, 1);
        if (hlat(s, x->i) == ')') {
            hltoken(x, HL_VAR, x->i+1);
        }
    } else if (hlidstart(d)) {
        hltoken(x, HL_VAR, hlidend(s, i+1));
    } else if (digit(d) || hlin(d, "@*#?$!-")) {
        hltoken(x, HL_VAR, i+2);
    } else {
        x->i++;
    }
}

static void hlshquote(Hl *x, i32 depth)  // "double-quoted string"
{
    std::string_view s = x->s;
    hlopen(x, HL_STRING);
    for (x->i++; x->i < std::ssize(s);) {
        u8 c = s[x->i];
        if (c == '\\') {
            x->i += 1 + (x->i+1 < std::ssize(s));
        } else if (c == '"') {
            x->i++;
            break;
        } else if (c == '$') {
            hlshdollar(x, depth);
        } else {
            x->i++;
        }
    }
    hlclose(x);
}

// Lex shell code, returning early at an unmatched ")" when nested.
static void hlshcode(Hl *x, i32 depth, b32 nested)
{
    std::string_view s      = x->s;
    i32 parens = 0;
    while (x->i < std::ssize(s)) {
        iz i = x->i;
        u8 c = s[i];
        if (c=='#' && (!i || whitespace((u8)s[i-1]) || hlin((u8)s[i-1], ";|&("))) {
            hltoken(x, HL_COMMENT, hleol(s, i));
        } else if (c == '\\') {
            x->i += 1 + (i+1 < std::ssize(s));
        } else if (c=='\'' || c=='`') {
            iz end = i + 1;
            for (; end<std::ssize(s) && (u8)s[end]!=c; end++) {}
            hltoken(x, HL_STRING, end + (end<std::ssize(s)));
        } else if (c == '"') {
            hlshquote(x, depth);
        } else if (c == '$') {
            hlshdollar(x, depth);
        } else if (c == '(') {
            parens++;
            x->i++;
        } else if (c == ')') {
            if (nested && !parens) {
                return;
            }
            parens--;
            x->i++;
        } else if (hlshword(c)) {
            iz end = i;
            for (; end<std::ssize(s) && hlshword((u8)s[end]); end++) {}
            iz name = hlidend(s, i);
            if (hlidstart(c) && hlat(s, name)=='=') {
                hltoken(x, HL_VAR, name);  // NAME=value
            } else if (hlmember(HlWords(shkeywords), std::string_view(s.data()+i, s.data()+end))) {
                hltoken(x, HL_KEYWORD, end);
            }
            x->i = end;
        } else {
            x->i++;
        }
    }
}

static void lexsh(Hl *x, HlLang *)
{
    hlshcode(x, 0, 0);
}


// Make: variables, rules, recipes (tab or space indented)

// End of the $ reference at s[i], like $@ or $(CC), not beyond end.
static iz hlmakeref(std::string_view s, iz i, iz end)
{
    u8 open  = hlat(s, i+1);
    u8 close = open=='(' ? ')' : '}';
    iz j     = i + 2;
    if (open=='(' || open=='{') {
        i32 nest = 1;
        for (; j<end && ((u8)s[j]!=close || --nest); j++) {
            nest += (u8)s[j] == open;
        }
        j++;
    }
    return j<end ? j : end;
}

// References, comments, and quoted strings up to end.
static void hlmakevalue(Hl *x, iz end)
{
    std::string_view s = x->s;
    while (x->i < end) {
        iz i = x->i;
        u8 c = s[i];
        if (c == '$') {
            hltoken(x, HL_VAR, hlmakeref(s, i, end));
        } else if (c=='#' && (i==0 || whitespace((u8)s[i-1]))) {
            hltoken(x, HL_COMMENT, end);
        } else if (c=='"' || c=='\'') {
            iz j = i + 1;
            for (; j<end && (u8)s[j]!=c; j++) {}
            hltoken(x, HL_STRING, j<end ? j+1 : end);
        } else {
            x->i++;
        }
    }
}

// A line outside of recipes. Returns true if it is a rule.
static b32 hlmakeline(Hl *x, iz eol, b32 rule)
{
    std::string_view s    = x->s;
    iz  beg  = hlblank(s, x->i);
    iz  word = beg;
    for (; word<eol && !whitespace((u8)s[word]); word++) {}
    x->i = beg;
    if (hlmember(HlWords(makedirectives), std::string_view(s.data()+beg, s.data()+word))) {
        hltoken(x, HL_KEYWORD, word);
        hlmakevalue(x, eol);
        return rule;
    }

    // Find the first ':' or '=' outside of references
    iz  sep  = beg;
    i32 nest = 0;
    for (; sep < eol; sep++) {
        u8 c = s[sep];
        if (c=='$' && hlin(hlat(s, sep+1), "({")) {
            nest++;
            sep++;
        } else if (nest && hlin(c, ")}")) {
            nest--;
        } else if (!nest && (c==':' || c=='=' || c=='#')) {
            break;
        }
    }
    if (sep==eol || s[sep]=='#') {
        hlmakevalue(x, eol);
        return rule;
    }

    u8  c      = s[sep];
    b32 assign = c=='=' || hlat(s, sep+1)=='=' || hlhas(s, sep, "::=");
    iz  name   = sep - (c=='=' && sep>beg && hlin((u8)s[sep-1], "+?!"));
    for (; name>beg && whitespace((u8)s[name-1]); name--) {}
    if (assign) {
        hltoken(x, HL_VAR, name);
        hlmakevalue(x, eol);
        return 0;
    }

    while (x->i < name) {  // targets, like .POSIX or foo-$(V).tar
        iz i = x->i;
        u8 c = s[i];
        if (whitespace(c)) {
            x->i++;
            continue;
        } else if (c == '$') {
            hltoken(x, HL_VAR, hlmakeref(s, i, name));
            continue;
        }
        iz  end     = i;
        b32 special = c=='.' && (i==beg || whitespace((u8)s[i-1]));
        for (; end<name && !whitespace((u8)s[end]) && s[end]!='$'; end++) {}
        hltoken(x, special ? HL_PREPROC : HL_FUNC, end);
    }
    x->i = sep;
    hlmakevalue(x, eol);
    return 1;
}

static void lexmake(Hl *x, HlLang *)
{
    std::string_view s    = x->s;
    b32 rule = 0;  // indented lines are recipes
    b32 cont = 0;  // this line continues the previous
    while (x->i < std::ssize(s)) {
        iz  bol  = x->i;
        iz  eol  = hleol(s, bol);
        std::string_view line = std::string_view(s.data()+bol, s.data()+eol);
        std::string_view text = trimleft(line);
        b32 more = std::ssize(line) && line[std::ssize(line)-1]=='\\';
        if (cont || (std::ssize(text) && text.data()!=line.data() && (rule || line[0]=='\t'))) {
            hlmakevalue(x, eol);  // recipe or continuation
        } else if (std::ssize(text) && text[0]=='#') {
            x->i = text.data() - s.data();
            hltoken(x, HL_COMMENT, eol);
        } else if (std::ssize(text)) {
            rule = hlmakeline(x, eol, rule);
        }
        x->i = eol + (eol < std::ssize(s));
        cont = more;
    }
}


// Line-oriented formats

static void lexdiff(Hl *x, HlLang *)
{
    std::string_view s      = x->s;
    b32 header = 0;  // between "diff" and the first hunk
    b32 files  = 0;  // previous line was a "---" file header
    while (x->i < std::ssize(s)) {
        iz  bol  = x->i;
        iz  eol  = hleol(s, bol);
        std::string_view line = std::string_view(s.data()+bol, s.data()+eol);
        std::string_view next = eol<std::ssize(s) ? std::string_view(s.data()+eol+1, s.data()+hleol(s, eol+1)) : std::string_view{};
        b32 top  = line.starts_with("--- ") && next.starts_with("+++ ");
        i32 cls  = HL_PLAIN;
        header |= line.starts_with("diff ");
        header &= !line.starts_with("@@");
        if (header || top || (files && line.starts_with("+++ "))) {
            cls = HL_HEADING;
        } else if (line.starts_with("@@")) {
            cls = HL_PREPROC;
        } else if (std::ssize(line) && line[0]=='+') {
            cls = HL_INSERTED;
        } else if (std::ssize(line) && line[0]=='-') {
            cls = HL_DELETED;
        } else if (std::ssize(line) && line[0]=='\\') {
            cls = HL_COMMENT;
        }
        files = top;
        hltoken(x, cls, eol);
        x->i += eol < std::ssize(s);
    }
}

static void lexini(Hl *x, HlLang *)
{
    std::string_view s = x->s;
    while (x->i < std::ssize(s)) {
        iz eol = hleol(s, x->i);
        x->i = hlblank(s, x->i);
        u8 c = hlat(s, x->i);
        if (c == '[') {
            iz end = x->i;
            for (; end<eol && s[end]!=']'; end++) {}
            hltoken(x, HL_KEYWORD, end + (end<eol));
        } else if (c==';' || c=='#') {
            hltoken(x, HL_COMMENT, eol);
        } else {
            iz eq = x->i;
            for (; eq<eol && s[eq]!='='; eq++) {}
            iz key = eq;
            for (; key>x->i && whitespace((u8)s[key-1]); key--) {}
            if (eq < eol) {
                hltoken(x, HL_VAR, key);
                x->i = hlblank(s, eq+1);
                if (hlin(hlat(s, x->i), "\"'")) {
                    hltoken(x, HL_STRING, eol);
                }
            }
        }
        x->i = eol + (eol < std::ssize(s));
    }
}

// pkg-config: "name=value" variables and "Field: value" keywords
static void lexpc(Hl *x, HlLang *)
{
    std::string_view s = x->s;
    while (x->i < std::ssize(s)) {
        iz eol = hleol(s, x->i);
        x->i = hlblank(s, x->i);
        iz end = x->i;
        for (; end<eol && (hlidchar((u8)s[end]) || hlin((u8)s[end], ".-")); end++) {}
        u8 sep = hlat(s, hlblank(s, end));  // "name = value" is allowed
        if (hlat(s, x->i) == '#') {
            hltoken(x, HL_COMMENT, eol);
        } else if (end>x->i && (sep==':' || sep=='=')) {
            hltoken(x, sep==':' ? HL_KEYWORD : HL_VAR, end);
        }
        while (x->i < eol) {
            iz i = x->i;
            if (hlhas(s, i, "${")) {
                iz j = i;
                for (; j<eol && s[j]!='}'; j++) {}
                hltoken(x, HL_VAR, j + (j<eol));
            } else if (s[i] == '"') {
                iz j = i + 1;
                for (; j<eol && s[j]!='"'; j++) {}
                hltoken(x, HL_STRING, j + (j<eol));
            } else {
                x->i++;
            }
        }
        x->i = eol + (eol < std::ssize(s));
    }
}

// Wavefront OBJ: a keyword then numbers per line
static void lexobj(Hl *x, HlLang *)
{
    std::string_view s = x->s;
    while (x->i < std::ssize(s)) {
        iz eol = hleol(s, x->i);
        x->i = hlblank(s, x->i);
        if (hlat(s, x->i) == '#') {
            hltoken(x, HL_COMMENT, eol);
        } else {
            iz end = x->i;
            for (; end<eol && !whitespace((u8)s[end]); end++) {}
            hltoken(x, HL_KEYWORD, end);
        }
        while (x->i < eol) {
            iz  i = x->i;
            iz  j = i + hlin((u8)s[i], "+-");
            b32 n = digit(hlat(s, j)) || (hlat(s, j)=='.' && digit(hlat(s, j+1)));
            b32 w = i==0 || !hlidchar((u8)s[i-1]);
            if (n && w) {
                hltoken(x, HL_NUMBER, hlnumber(s, j));
            } else if (hlidchar((u8)s[i])) {
                x->i = hlidend(s, i);
            } else {
                x->i++;
            }
        }
        x->i = eol + (eol < std::ssize(s));
    }
}

// Windows batch files
static void lexbat(Hl *x, HlLang *)
{
    std::string_view s     = x->s;
    b32 first = 1;  // next word is a command
    while (x->i < std::ssize(s)) {
        iz i = x->i;
        u8 c = s[i];
        if (c=='\n' || hlin(c, "&|(")) {
            first = 1;
            x->i++;
        } else if (whitespace(c) || (c=='@' && first)) {
            x->i++;
        } else if (first && hlhas(s, i, "::")) {
            hltoken(x, HL_COMMENT, hleol(s, i));
        } else if (c == '"') {
            iz end = i + 1;
            for (; end<std::ssize(s) && s[end]!='"' && s[end]!='\n'; end++) {}
            hltoken(x, HL_STRING, end + (hlat(s, end)=='"'));
            first = 0;
        } else if (c == '%') {
            u8 d   = hlat(s, i+1);
            iz end = i + 1;
            if (d=='*' || digit(d)) {
                end = i + 2;
            } else if (d=='%' && letter(hlat(s, i+2))) {
                end = i + 3;
            } else if (d == '~') {
                for (end = i+2; end<std::ssize(s) && letter((u8)s[end]); end++) {}
                end += digit(hlat(s, end));
            } else {
                for (; end<std::ssize(s) && (hlidchar((u8)s[end]) || s[end]==' '); end++) {}
                end = hlat(s, end)=='%' ? end+1 : i+1;  // %name%
            }
            hltoken(x, end>i+1 ? HL_VAR : HL_PLAIN, end);
            first = 0;
        } else if (hlidchar(c)) {
            iz  end = hlidend(s, i);
            std::string_view w   = std::string_view(s.data()+i, s.data()+end);
            if (first && hlword(HlWords(basicrem), w, 1)) {
                hltoken(x, HL_COMMENT, hleol(s, i));
            } else {
                b32 keyword = hlword(HlWords(batkeywords), w, 1);
                hltoken(x, keyword ? HL_KEYWORD : HL_PLAIN, end);
            }
            first = 0;
        } else {
            x->i++;
            first = 0;
        }
    }
}

// Tokens in a YAML value up to end.
static void hlyamlvalue(Hl *x, iz end)
{
    std::string_view s = x->s;
    while (x->i < end) {
        iz i = x->i;
        u8 c = s[i];
        if (whitespace(c) || hlin(c, "[]{},")) {
            x->i++;
        } else if (c == '#') {
            hltoken(x, HL_COMMENT, end);
        } else if (c=='"' || c=='\'') {
            iz j = i + 1;
            for (; j<end && (u8)s[j]!=c; j++) {
                j += c=='"' && s[j]=='\\' && j+1<end;
            }
            hltoken(x, HL_STRING, j + (j<end));
        } else {
            iz j = i;  // plain scalar, up to a flow indicator
            for (; j<end && !whitespace((u8)s[j]) && !hlin((u8)s[j], ",[]{}"); j++) {}
            std::string_view w  = std::string_view(s.data()+i, s.data()+j);
            iz  d  = hlin(c, "+-");
            b32 n  = d<std::ssize(w) && (digit(w[d]) || w[d]=='.') && hlnumber(s, i+d)==j;
            i32 cl = HL_PLAIN;
            if (n) {
                cl = HL_NUMBER;
            } else if (hlmember(HlWords(yamlkeywords), w) || w=="~") {
                cl = HL_KEYWORD;
            } else if (hlin(c, "&*!") && std::ssize(w)>1) {
                cl = HL_VAR;  // anchor, alias, tag
            }
            hltoken(x, cl, j);
        }
    }
}

static void lexyaml(Hl *x, HlLang *)
{
    std::string_view s     = x->s;
    iz  block = -1;  // indentation of a line that opened a block scalar
    while (x->i < std::ssize(s)) {
        iz bol    = x->i;
        iz eol    = hleol(s, bol);
        iz indent = 0;
        for (; bol+indent<eol && s[bol+indent]==' '; indent++) {}
        iz i = bol + indent;

        if (block>=0 && (i==eol || indent>block)) {
            x->i = i;
            hltoken(x, HL_STRING, eol);  // block scalar content
            x->i = eol + (eol < std::ssize(s));
            continue;
        }
        block = -1;

        x->i = i;
        std::string_view rest = trimright(std::string_view(s.data()+i, s.data()+eol));
        if (bol==i && (rest=="---" || rest=="...")) {
            hltoken(x, HL_PREPROC, eol);
        } else if (hlat(s, i) == '#') {
            hltoken(x, HL_COMMENT, eol);
        } else {
            for (; hlat(s, x->i)=='-' && (x->i+1==eol || whitespace(hlat(s, x->i+1)));) {
                x->i = hlblank(s, x->i+1);  // sequence entries
            }
            // A key is a plain or quoted scalar followed by ": "
            iz k = x->i;
            u8 q = hlat(s, k);
            if (q=='"' || q=='\'') {
                for (k++; k<eol && (u8)s[k]!=q; k++) {}
                k += k < eol;
            } else {
                for (; k<eol && s[k]!='#' && !hlin((u8)s[k], "[{"); k++) {
                    if (s[k]==':' && (k+1==eol || whitespace((u8)s[k+1]))) break;
                }
            }
            if (k>x->i && hlat(s, k)==':' && (k+1==eol || whitespace(hlat(s, k+1)))) {
                hltoken(x, HL_VAR, k);
                x->i = k + 1;
            }
            // Block scalar indicator: | or > with optional modifiers
            iz v = hlblank(s, x->i);
            if (hlin(hlat(s, v), "|>")) {
                iz m = v + 1;
                for (; m<eol && hlin((u8)s[m], "+-0123456789"); m++) {}
                iz after = hlblank(s, m);
                if (after==eol || s[after]=='#') {
                    x->i = v;
                    hltoken(x, HL_PREPROC, m);
                    block = indent;
                }
            }
            hlyamlvalue(x, eol);
        }
        x->i = eol + (eol < std::ssize(s));
    }
}


// XML and HTML

static b32 hlmarkupdelim(u8 c)  // ends a tag or attribute name
{
    return whitespace(c) || hlin(c, "=>/<\"'");
}

static void lexmarkup(Hl *x, HlLang *)
{
    std::string_view s = x->s;
    while (x->i < std::ssize(s)) {
        iz i = x->i;
        u8 c = s[i];
        u8 d = hlat(s, i+1);
        if (c == '&') {  // entity
            iz end = i + 1;
            for (; end<std::ssize(s) && (alnum((u8)s[end]) || s[end]=='#'); end++) {}
            if (end>i+1 && hlat(s, end)==';') {
                hltoken(x, HL_VAR, end+1);
            } else {
                x->i++;
            }
        } else if (c != '<') {
            x->i++;
        } else if (hlhas(s, i, "<!--")) {
            uz end = s.find("-->", i+4);
            hltoken(x, HL_COMMENT, end==s.npos ? std::ssize(s) : (iz)end+3);
        } else if (d=='?' || d=='!') {  // <?xml ...?> and <!DOCTYPE ...>
            iz end = i;
            for (; end<std::ssize(s) && s[end]!='>'; end++) {}
            hltoken(x, HL_PREPROC, end + (end<std::ssize(s)));
        } else if (hlidstart(d) || (d=='/' && hlidstart(hlat(s, i+2)))) {
            iz end = i + 1 + (d=='/');
            for (; end<std::ssize(s) && !hlmarkupdelim((u8)s[end]); end++) {}
            hltoken(x, HL_KEYWORD, end);
            while (x->i < std::ssize(s)) {  // attributes
                iz j = x->i;
                u8 a = s[j];
                if (a == '>') {
                    hltoken(x, HL_KEYWORD, j+1);
                    break;
                } else if (a=='/' && hlat(s, j+1)=='>') {
                    hltoken(x, HL_KEYWORD, j+2);
                    break;
                } else if (a == '<') {
                    break;  // malformed
                } else if (a=='"' || a=='\'') {
                    iz end = j + 1;
                    for (; end<std::ssize(s) && (u8)s[end]!=a; end++) {}
                    hltoken(x, HL_STRING, end + (end<std::ssize(s)));
                } else if (whitespace(a) || hlin(a, "=/")) {
                    x->i++;
                } else {  // name, or unquoted value
                    iz end = j;
                    for (; end<std::ssize(s) && !hlmarkupdelim((u8)s[end]); end++) {}
                    hltoken(x, hlat(s, j-1)=='=' ? HL_STRING : HL_VAR, end);
                }
            }
        } else {
            x->i++;
        }
    }
}


// Language table

static HlLang hllang(std::string_view tag)
{
    HlLang l = {};
    l.lex    = lexgeneric;
    l.escape = '\\';

    if (tag=="c" || tag=="c++" || tag=="cpp") {
        b32 cpp    = tag != "c";
        l.comment  = "//";
        l.open     = "/*";
        l.close    = "*/";
        l.quotes   = "\"'";
        l.flags    = LEX_CPP | LEX_FUNCDEF | LEX_TSUFFIX | LEX_CAMEL;
        l.flags   |= cpp ? LEX_CLASSES : 0;
        l.keywords = cpp ? HlWords(cppkeywords) : HlWords(ckeywords);
        l.types    = HlWords(ctypes);
        l.tdefs    = cpp ? HlWords(cpptdefs) : HlWords(ctdefs);
        l.prefixes = HlWords(cprefixes);

    } else if (tag == "glsl") {
        l.comment  = "//";
        l.open     = "/*";
        l.close    = "*/";
        l.quotes   = "\"";
        l.flags    = LEX_CPP | LEX_FUNCDEF;
        l.keywords = HlWords(glslkeywords);
        l.types    = HlWords(glsltypes);
        l.tdefs    = HlWords(ctdefs);

    } else if (tag == "java") {
        l.comment  = "//";
        l.open     = "/*";
        l.close    = "*/";
        l.quotes   = "\"'";
        l.flags    = LEX_FUNCDEF | LEX_METHODS | LEX_CAMEL | LEX_AT;
        l.keywords = HlWords(javakeywords);
        l.types    = HlWords(javatypes);
        l.defs     = HlWords(javadefs);
        l.tdefs    = HlWords(javatdefs);

    } else if (tag=="javascript" || tag=="js") {
        l.comment  = "//";
        l.open     = "/*";
        l.close    = "*/";
        l.quotes   = "\"'`";
        l.flags    = LEX_REGEX | LEX_IDDOLLAR;
        l.keywords = HlWords(jskeywords);
        l.defs     = HlWords(jsdefs);

    } else if (tag == "go") {
        l.comment  = "//";
        l.open     = "/*";
        l.close    = "*/";
        l.quotes   = "\"'`";
        l.keywords = HlWords(gokeywords);
        l.types    = HlWords(gotypes);
        l.defs     = HlWords(godefs);

    } else if (tag=="py" || tag=="python") {
        l.comment  = "#";
        l.quotes   = "\"'";
        l.flags    = LEX_TRIPLE | LEX_AT;
        l.keywords = HlWords(pykeywords);
        l.types    = HlWords(pytypes);
        l.defs     = HlWords(pydefs);
        l.prefixes = HlWords(pyprefixes);

    } else if (tag == "lua") {
        l.comment  = "--";
        l.open     = "--[[";
        l.close    = "]]";
        l.quotes   = "\"'";
        l.keywords = HlWords(luakeywords);
        l.defs     = HlWords(luadefs);

    } else if (tag == "php") {
        l.comment  = "//";
        l.comment2 = "#";
        l.open     = "/*";
        l.close    = "*/";
        l.quotes   = "\"'";
        l.flags    = LEX_DOLLAR;
        l.keywords = HlWords(phpkeywords);
        l.defs     = HlWords(phpdefs);

    } else if (tag == "perl") {
        l.comment  = "#";
        l.quotes   = "\"'";
        l.flags    = LEX_DOLLAR | LEX_PERL | LEX_REGEX;
        l.keywords = HlWords(perlkeywords);
        l.defs     = HlWords(perldefs);

    } else if (tag == "ruby") {
        l.comment  = "#";
        l.quotes   = "\"'";
        l.flags    = LEX_SYMBOL | LEX_REGEX;
        l.keywords = HlWords(rubykeywords);
        l.defs     = HlWords(rubydefs);

    } else if (tag == "julia") {
        l.comment  = "#";
        l.open     = "#=";
        l.close    = "=#";
        l.quotes   = "\"'";
        l.flags    = LEX_TRIPLE | LEX_TRANSPOSE | LEX_CAMEL | LEX_AT;
        l.keywords = HlWords(juliakeywords);
        l.defs     = HlWords(juliadefs);

    } else if (tag=="matlab" || tag=="octave") {
        l.comment  = "%";
        l.comment2 = "#";
        l.quotes   = "\"'";
        l.escape   = 0;
        l.flags    = LEX_TRANSPOSE | LEX_DOUBLING | LEX_OUTPUTS;
        l.keywords = HlWords(matlabkeywords);
        l.defs     = HlWords(matlabdefs);

    } else if (tag == "sql") {
        l.comment  = "--";
        l.open     = "/*";
        l.close    = "*/";
        l.quotes   = "\"'";
        l.escape   = 0;
        l.flags    = LEX_CASEFOLD | LEX_DOUBLING;
        l.keywords = HlWords(sqlkeywords);
        l.types    = HlWords(sqltypes);

    } else if (tag == "qbasic") {
        l.comment  = "'";
        l.quotes   = "\"";
        l.escape   = 0;
        l.flags    = LEX_CASEFOLD | LEX_BASIC;
        l.keywords = HlWords(basickeywords);
        l.types    = HlWords(basictypes);
        l.defs     = HlWords(basicdefs);

    } else if (tag == "gnuplot") {
        l.comment  = "#";
        l.quotes   = "\"'";
        l.keywords = HlWords(gnuplotkeywords);

    } else if (tag == "vim") {
        l.quotes   = "\"'";
        l.flags    = LEX_VIM;
        l.keywords = HlWords(vimkeywords);

    } else if (tag == "json") {
        l.quotes   = "\"";
        l.flags    = LEX_KEYS;
        l.keywords = HlWords(jsonkeywords);

    } else if (tag=="cl" || tag=="lisp" || tag=="elisp" || tag=="scheme" ||
               tag=="clojure" || tag=="wat") {
        l.lex = lexlisp;
        if (tag == "wat") {
            l.flags = LEX_WAT;
        } else if (tag!="scheme" && tag!="clojure") {
            l.flags = LEX_ELISP;  // "cl" is used for Emacs Lisp too
        }

    } else if (tag == "nasm") {
        l.lex   = lexasm;
        l.flags = LEX_NASM;
    } else if (tag == "att") {
        l.lex   = lexasm;
        l.flags = LEX_ATT;
    } else if (tag == "aarch64") {
        l.lex   = lexasm;
        l.flags = LEX_ARM;

    } else if (tag=="sh" || tag=="bash") {
        l.lex = lexsh;
    } else if (tag=="make" || tag=="makefile") {
        l.lex = lexmake;
    } else if (tag=="diff" || tag=="udiff") {
        l.lex = lexdiff;
    } else if (tag=="xml" || tag=="html") {
        l.lex = lexmarkup;
    } else if (tag == "yaml") {
        l.lex = lexyaml;
    } else if (tag == "ini") {
        l.lex = lexini;
    } else if (tag == "pc") {
        l.lex = lexpc;
    } else if (tag == "obj") {
        l.lex = lexobj;
    } else if (tag == "bat") {
        l.lex = lexbat;

    } else {
        l.lex = 0;
    }
    return l;
}


// True if stripping the spans from html and unescaping it gives code.
static b32 hlroundtrip(std::string_view html, std::string_view code)
{
    iz  j     = 0;
    i32 depth = 0;
    for (iz i = 0; i < std::ssize(html);) {
        std::string_view rest = html.substr(i);
        if (rest.starts_with("</span>")) {
            depth--;
            i += 7;
        } else if (rest.starts_with("<span class=\"")) {
            uz end = rest.find("\">");
            if (end==rest.npos || end<14) return 0;
            depth++;
            i += (iz)end + 2;
        } else {
            u8 c = rest[0];
            iz n = 1;
            if (c=='<' || c=='>') {
                return 0;
            } else if (rest.starts_with("&amp;")) {
                n = 5;
            } else if (rest.starts_with("&lt;")) {
                c = '<';
                n = 4;
            } else if (rest.starts_with("&gt;")) {
                c = '>';
                n = 4;
            } else if (c == '&') {
                return 0;
            }
            if (j>=std::ssize(code) || (u8)code[j]!=c) return 0;
            j++;
            i += n;
        }
        if (depth < 0) return 0;
    }
    return !depth && j==std::ssize(code);
}

// Write code to b, HTML-escaped, with tokens wrapped in <span class="X">.
// Returns false if the language is unknown, in which case the code is
// written escaped without highlighting. No language is not unknown.
static b32 highlight(std::string *b, std::string_view lang, std::string_view code)
{
    HlLang l = hllang(lang);
    if (!l.lex) {
        printhtml(b, code);
        return lang.empty();
    }

    iz start = std::ssize(*b);
    Hl x     = {};
    x.b = b;
    x.s = code;
    l.lex(&x, &l);
    assert(x.i <= std::ssize(code));
    x.i = std::ssize(code);
    hlflush(&x);

    #ifndef NDEBUG
    assert(hlroundtrip(std::string_view(*b).substr(start), code));
    #endif
    (void)start;
    return 1;
}
