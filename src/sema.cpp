#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>
#include "ast.hpp"

struct Sym {
    std::string name;
    bool is_arr;
};

static std::vector<Sym> syms;
static int errors, in_loop;
static bool fn_void;
static Node *prog;

static void err(int line, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::fprintf(stderr, "semantic error at line %d: ", line);
    std::vfprintf(stderr, fmt, ap);
    std::fputc('\n', stderr);
    va_end(ap);
    errors++;
}

static int lookup(const std::string &n) {
    for (int i = static_cast<int>(syms.size()) - 1; i >= 0; i--)
        if (syms[i].name == n) return i;
    return -1;
}

static void declare(const std::string &n, bool is_arr, int line, int mark) {
    for (int i = static_cast<int>(syms.size()) - 1; i >= mark; i--)
        if (syms[i].name == n) { err(line, "redeclaration of '%s'", n.c_str()); return; }
    syms.push_back({n, is_arr});
}

Node *find_func(Node *root, const std::string &name) {
    for (Node *k : root->kids)
        if (k->kind == N_FUNC && k->text == name) return k;
    return nullptr;
}

static void expr(Node *n, bool want_value);

static bool is_array_id(Node *a) {
    if (a->kind != N_ID) return false;
    int s = lookup(a->text);
    return s >= 0 && syms[s].is_arr;
}

static void lvalue(Node *n) {
    int s = lookup(n->text);
    if (s < 0) err(n->line, "undeclared identifier '%s'", n->text.c_str());
    if (n->kind == N_ID) {
        if (s >= 0 && syms[s].is_arr) err(n->line, "array '%s' used without index", n->text.c_str());
    } else {
        if (s >= 0 && !syms[s].is_arr) err(n->line, "'%s' is not an array", n->text.c_str());
        expr(n->kids[0], true);
    }
}

static void call(Node *n, bool want_value) {
    if (n->text == "printf") {
        if (n->size() < 1 || n->kids[0]->kind != N_STR) err(n->line, "printf: first argument must be a string literal");
        for (int i = 1; i < n->size(); i++) {
            if (n->kids[i]->kind == N_STR) continue;
            if (is_array_id(n->kids[i])) err(n->line, "printf: array passed as value");
            else expr(n->kids[i], true);
        }
        return;
    }
    Node *f = find_func(prog, n->text);
    if (!f) { err(n->line, "call to undefined function '%s'", n->text.c_str()); return; }
    Node *ps = f->kids[1];
    if (n->size() != ps->size())
        err(n->line, "function '%s' expects %d argument(s), got %d", n->text.c_str(), ps->size(), n->size());
    if (want_value && f->kids[0]->text == "void")
        err(n->line, "void function '%s' used as value", n->text.c_str());
    for (int i = 0; i < n->size(); i++) {
        bool arr = is_array_id(n->kids[i]);
        if (i < ps->size()) {
            if (ps->kids[i]->ival && !arr) err(n->line, "argument %d of '%s' must be an array", i + 1, n->text.c_str());
            if (!ps->kids[i]->ival && arr) err(n->line, "argument %d of '%s': array passed where int expected", i + 1, n->text.c_str());
        }
        if (!arr) expr(n->kids[i], true);
    }
}

static void expr(Node *n, bool want_value) {
    switch (n->kind) {
    case N_STR: err(n->line, "string literal is allowed only as printf argument"); break;
    case N_ID:
    case N_INDEX: lvalue(n); break;
    case N_BINOP: expr(n->kids[0], true); expr(n->kids[1], true); break;
    case N_UNOP: expr(n->kids[0], true); break;
    case N_INCDEC: lvalue(n->kids[0]); break;
    case N_ASSIGN: lvalue(n->kids[0]); expr(n->kids[1], true); break;
    case N_COND: for (int i = 0; i < 3; i++) expr(n->kids[i], true); break;
    case N_CALL: call(n, want_value); break;
    default: break;
    }
}

static void stmt(Node *n, int mark);

static void block(Node *b) {
    int mark = static_cast<int>(syms.size());
    for (Node *k : b->kids) stmt(k, mark);
    syms.resize(mark);
}

static void stmt(Node *n, int mark) {
    switch (n->kind) {
    case N_BLOCK: block(n); break;
    case N_DECLS:
        for (int i = 1; i < n->size(); i++) {
            Node *v = n->kids[i];
            if (v->size()) {
                Node *init = v->kids[0];
                if (init->kind == N_INITLIST) {
                    for (Node *e : init->kids) expr(e, true);
                    if (v->ival > 0 && init->size() > v->ival) err(v->line, "too many initializers for '%s'", v->text.c_str());
                    if (v->ival == 0) v->ival = init->size();
                } else expr(init, true);
            } else if (v->ival == 0) err(v->line, "array '%s' has no size", v->text.c_str());
            if (n->kids[0]->text == "void") err(v->line, "variable '%s' declared void", v->text.c_str());
            declare(v->text, v->ival >= 0, v->line, mark);
        }
        break;
    case N_EXPRSTMT: expr(n->kids[0], false); break;
    case N_IF:
        expr(n->kids[0], true);
        stmt(n->kids[1], static_cast<int>(syms.size()));
        if (n->size() > 2) stmt(n->kids[2], static_cast<int>(syms.size()));
        break;
    case N_WHILE:
        expr(n->kids[0], true);
        in_loop++;
        stmt(n->kids[1], static_cast<int>(syms.size()));
        in_loop--;
        break;
    case N_DOWHILE:
        in_loop++;
        stmt(n->kids[0], static_cast<int>(syms.size()));
        in_loop--;
        expr(n->kids[1], true);
        break;
    case N_FOR: {
        int m = static_cast<int>(syms.size());
        if (n->kids[0]->kind == N_DECLS) stmt(n->kids[0], m); else expr(n->kids[0], false);
        expr(n->kids[1], true);
        expr(n->kids[2], false);
        in_loop++;
        stmt(n->kids[3], static_cast<int>(syms.size()));
        in_loop--;
        syms.resize(m);
        break;
    }
    case N_RETURN:
        if (n->size() && fn_void) err(n->line, "void function returns a value");
        if (!n->size() && !fn_void) err(n->line, "non-void function returns no value");
        if (n->size()) expr(n->kids[0], true);
        break;
    case N_BREAK: if (!in_loop) err(n->line, "'break' outside of a loop"); break;
    case N_CONTINUE: if (!in_loop) err(n->line, "'continue' outside of a loop"); break;
    default: break;
    }
}

int sema_check(Node *root) {
    prog = root;
    errors = 0;
    syms.clear();
    for (int i = 0; i < root->size(); i++)
        if (root->kids[i]->kind == N_FUNC)
            for (int j = 0; j < i; j++)
                if (root->kids[j]->kind == N_FUNC && root->kids[j]->text == root->kids[i]->text)
                    err(root->kids[i]->line, "redefinition of function '%s'", root->kids[i]->text.c_str());
    for (Node *e : root->kids) {
        if (e->kind == N_DECLS) { stmt(e, 0); continue; }
        int mark = static_cast<int>(syms.size());
        in_loop = 0;
        fn_void = e->kids[0]->text == "void";
        Node *ps = e->kids[1];
        for (Node *p : ps->kids) declare(p->text, p->ival != 0, p->line, mark);
        block(e->kids[2]);
        syms.resize(mark);
    }
    return errors;
}
