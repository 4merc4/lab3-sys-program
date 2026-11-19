#include <cctype>
#include <cstdlib>
#include <cstring>
#include "ast.hpp"
#include "parser.tab.hpp"

int yylineno = 1;
int lex_errors = 0;
static const char *p;

void lexer_init(const char *src) {
    p = src;
    yylineno = 1;
    lex_errors = 0;
}

static const struct { const char *w; int tok; } KW[] = {
    {"int", INT}, {"void", VOID}, {"if", IF}, {"else", ELSE}, {"while", WHILE}, {"do", DO},
    {"for", FOR}, {"return", RETURN}, {"break", BREAK}, {"continue", CONTINUE}, {nullptr, 0}
};

static const struct { const char *s; int tok; } OPS[] = {
    {"<<=", ASSIGN_OP}, {">>=", ASSIGN_OP},
    {"==", EQ}, {"!=", NE}, {"<=", LE}, {">=", GE}, {"&&", AND}, {"||", OR},
    {"<<", SHL}, {">>", SHR}, {"++", INC}, {"--", DEC},
    {"+=", ASSIGN_OP}, {"-=", ASSIGN_OP}, {"*=", ASSIGN_OP}, {"/=", ASSIGN_OP},
    {"%=", ASSIGN_OP}, {"&=", ASSIGN_OP}, {"|=", ASSIGN_OP}, {"^=", ASSIGN_OP},
    {nullptr, 0}
};

static void lex_error(const char *msg) {
    std::fprintf(stderr, "lexical error at line %d: %s\n", yylineno, msg);
    lex_errors++;
}

static int escape(char c) {
    switch (c) {
    case 'n': return '\n';
    case 't': return '\t';
    case 'r': return '\r';
    case '0': return 0;
    case '\\': return '\\';
    case '\'': return '\'';
    case '"': return '"';
    default: lex_error("unknown escape sequence"); return c;
    }
}

int yylex() {
    for (;;) {
        if (*p == '\n') { yylineno++; p++; }
        else if (std::isspace(static_cast<unsigned char>(*p))) p++;
        else if (p[0] == '/' && p[1] == '/') { while (*p && *p != '\n') p++; }
        else if (p[0] == '/' && p[1] == '*') {
            p += 2;
            while (*p && !(p[0] == '*' && p[1] == '/')) { if (*p == '\n') yylineno++; p++; }
            if (!*p) { lex_error("unterminated comment"); return 0; }
            p += 2;
        } else break;
    }
    if (!*p) return 0;

    if (std::isalpha(static_cast<unsigned char>(*p)) || *p == '_') {
        const char *s = p;
        while (std::isalnum(static_cast<unsigned char>(*p)) || *p == '_') p++;
        size_t len = p - s;
        for (int i = 0; KW[i].w; i++)
            if (std::strlen(KW[i].w) == len && !std::strncmp(KW[i].w, s, len)) return KW[i].tok;
        yylval.str = strndup(s, len);
        return ID;
    }
    if (std::isdigit(static_cast<unsigned char>(*p))) {
        char *end;
        yylval.ival = std::strtol(p, &end, (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) ? 16 : 10);
        p = end;
        if (std::isalpha(static_cast<unsigned char>(*p)) || *p == '_') {
            lex_error("invalid number suffix");
            while (std::isalnum(static_cast<unsigned char>(*p))) p++;
        }
        return NUM;
    }
    if (*p == '\'') {
        p++;
        int v;
        if (*p == '\\') { p++; v = escape(*p); p++; } else { v = static_cast<unsigned char>(*p); p++; }
        if (*p != '\'') lex_error("bad character constant"); else p++;
        yylval.ival = v;
        return CHARLIT;
    }
    if (*p == '"') {
        std::string buf;
        p++;
        while (*p && *p != '"' && *p != '\n') {
            if (*p == '\\') { p++; buf += static_cast<char>(escape(*p)); p++; } else buf += *p++;
            if (buf.size() > 1000) break;
        }
        if (*p == '"') p++; else lex_error("unterminated string");
        yylval.str = strdup(buf.c_str());
        return STRING;
    }
    for (int i = 0; OPS[i].s; i++) {
        size_t l = std::strlen(OPS[i].s);
        if (!std::strncmp(p, OPS[i].s, l)) {
            if (OPS[i].tok == ASSIGN_OP) yylval.str = strdup(OPS[i].s);
            p += l;
            return OPS[i].tok;
        }
    }
    if (std::strchr("+-*/%<>=!&|^~?:;,(){}[]", *p)) return static_cast<unsigned char>(*p++);
    char m[40];
    std::snprintf(m, sizeof m, "unexpected character '%c'", *p);
    lex_error(m);
    p++;
    return yylex();
}
