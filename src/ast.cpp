#include "ast.hpp"

static const char *const KN[] = {
    "Program", "Func", "Type", "Params", "Param", "Decls", "VarDecl", "InitList",
    "Block", "ExprStmt", "Empty", "If", "While", "DoWhile", "For",
    "Return", "Break", "Continue",
    "Num", "Char", "Str", "Id", "BinOp", "UnOp", "IncDec", "Assign", "Cond",
    "Index", "Call"
};

const char *kind_name(NodeKind k) { return KN[k]; }

Node *mk(NodeKind k, const std::string &text, long ival, int line) {
    Node *n = new Node;
    n->kind = k;
    n->text = text;
    n->ival = ival;
    n->line = line;
    return n;
}

Node *add(Node *p, Node *c) {
    if (c) p->kids.push_back(c);
    return p;
}

Node *mk1(NodeKind k, const std::string &t, long iv, int l, Node *a) { return add(mk(k, t, iv, l), a); }
Node *mk2(NodeKind k, const std::string &t, long iv, int l, Node *a, Node *b) { return add(mk1(k, t, iv, l, a), b); }
Node *mk3(NodeKind k, const std::string &t, long iv, int l, Node *a, Node *b, Node *c) { return add(mk2(k, t, iv, l, a, b), c); }

std::string node_label(const Node *n) {
    std::string k = kind_name(n->kind);
    switch (n->kind) {
    case N_NUM:
    case N_CHAR:
        return k + " " + std::to_string(n->ival);
    case N_VARDECL:
        if (n->ival < 0) return k + " " + n->text;
        if (n->ival == 0) return k + " " + n->text + "[]";
        return k + " " + n->text + "[" + std::to_string(n->ival) + "]";
    case N_PARAM:
        return k + " " + n->text + (n->ival ? "[]" : "");
    case N_INCDEC:
        return k + " " + (n->ival ? "pre" : "post") + n->text;
    case N_STR:
        return k + " \"" + n->text + "\"";
    default:
        return n->text.empty() ? k : k + " " + n->text;
    }
}

static void tree_rec(const Node *n, FILE *f, const std::string &prefix, bool last, bool root) {
    std::string label = node_label(n);
    if (root) std::fprintf(f, "%s\n", label.c_str());
    else std::fprintf(f, "%s%s%s\n", prefix.c_str(), last ? "└── " : "├── ", label.c_str());
    std::string np = prefix + (root ? "" : (last ? "    " : "│   "));
    for (int i = 0; i < n->size(); i++) tree_rec(n->kids[i], f, np, i == n->size() - 1, false);
}

void print_tree(const Node *n, FILE *f) { tree_rec(n, f, "", true, true); }

void json_string(FILE *f, const std::string &s) {
    std::fputc('"', f);
    for (unsigned char c : s) {
        if (c == '"') std::fputs("\\\"", f);
        else if (c == '\\') std::fputs("\\\\", f);
        else if (c == '\n') std::fputs("\\n", f);
        else if (c == '\t') std::fputs("\\t", f);
        else if (c < 0x20) std::fprintf(f, "\\u%04x", c);
        else std::fputc(c, f);
    }
    std::fputc('"', f);
}

void print_json(const Node *n, FILE *f) {
    std::fputs("{\"name\":", f);
    json_string(f, node_label(n));
    std::fprintf(f, ",\"kind\":\"%s\",\"line\":%d", kind_name(n->kind), n->line);
    if (n->size()) {
        std::fputs(",\"children\":[", f);
        for (int i = 0; i < n->size(); i++) {
            if (i) std::fputc(',', f);
            print_json(n->kids[i], f);
        }
        std::fputc(']', f);
    }
    std::fputc('}', f);
}
