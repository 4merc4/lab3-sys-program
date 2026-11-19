#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "ast.hpp"

struct Val {
    long v = 0;
    long *a = nullptr;
    long len = 0;
    bool arr = false;
    bool owned = false;
};

struct Var {
    std::string name;
    Val val;
};

enum { ST_NORMAL, ST_BREAK, ST_CONTINUE, ST_RETURN };

static std::vector<Var> vars(65536);
static int sp, frame_base, nglobals, depth;
static Node *prog;
static long retval;

[[noreturn]] static void rt_error(int line, const char *fmt, ...) {
    va_list ap;
    std::fflush(stdout);
    std::fprintf(stderr, "runtime error at line %d: ", line);
    va_start(ap, fmt);
    std::vfprintf(stderr, fmt, ap);
    va_end(ap);
    std::fputc('\n', stderr);
    std::exit(2);
}

static Var *find_var(const std::string &n) {
    for (int i = sp - 1; i >= frame_base; i--) if (vars[i].name == n) return &vars[i];
    for (int i = nglobals - 1; i >= 0; i--) if (vars[i].name == n) return &vars[i];
    return nullptr;
}

static void push(const std::string &n, const Val &v) {
    if (sp >= 65536) rt_error(0, "variable stack overflow");
    vars[sp].name = n;
    vars[sp].val = v;
    sp++;
}

static void pop_to(int mark) {
    while (sp > mark) {
        sp--;
        if (vars[sp].val.owned) std::free(vars[sp].val.a);
    }
}

static long eval(Node *n);
static int exec(Node *n);

static long *ref(Node *lv) {
    Var *v = find_var(lv->text);
    if (!v) rt_error(lv->line, "undeclared '%s'", lv->text.c_str());
    if (lv->kind == N_ID) return &v->val.v;
    long i = eval(lv->kids[0]);
    if (i < 0 || i >= v->val.len)
        rt_error(lv->line, "index %ld out of bounds for '%s' (size %ld)", i, lv->text.c_str(), v->val.len);
    return &v->val.a[i];
}

static long binop(const std::string &op, long a, long b, int line) {
    switch (op[0]) {
    case '+': return a + b;
    case '-': return a - b;
    case '*': return a * b;
    case '/': if (!b) rt_error(line, "division by zero"); return a / b;
    case '%': if (!b) rt_error(line, "division by zero"); return a % b;
    case '&': return a & b;
    case '|': return a | b;
    case '^': return a ^ b;
    case '<':
        if (op.size() > 1 && op[1] == '<') {
            if (b < 0 || b > 62) rt_error(line, "bad shift count %ld", b);
            return a << b;
        }
        return op.size() > 1 && op[1] == '=' ? a <= b : a < b;
    case '>':
        if (op.size() > 1 && op[1] == '>') {
            if (b < 0 || b > 62) rt_error(line, "bad shift count %ld", b);
            return a >> b;
        }
        return op.size() > 1 && op[1] == '=' ? a >= b : a > b;
    case '=': return a == b;
    case '!': return a != b;
    }
    rt_error(line, "unknown operator %s", op.c_str());
}

static long do_printf(Node *c) {
    const std::string &f = c->kids[0]->text;
    int arg = 1;
    long count = 0;
    for (size_t i = 0; i < f.size(); i++) {
        if (f[i] != '%') { std::putchar(f[i]); count++; continue; }
        std::string spec = "%";
        i++;
        while (i < f.size() && std::strchr("-+ 0#123456789", f[i]) && spec.size() < 20) spec += f[i++];
        if (i < f.size() && f[i] == '%') { std::putchar('%'); count++; continue; }
        if (i >= f.size()) break;
        char conv = f[i];
        if (conv == 's') {
            if (arg >= c->size() || c->kids[arg]->kind != N_STR) rt_error(c->line, "printf: %%s needs a string literal");
            spec += 's';
            count += std::printf(spec.c_str(), c->kids[arg++]->text.c_str());
        } else if (std::strchr("dixXco", conv)) {
            if (arg >= c->size()) rt_error(c->line, "printf: too few arguments");
            long v = eval(c->kids[arg++]);
            if (conv == 'c') {
                spec += 'c';
                count += std::printf(spec.c_str(), static_cast<int>(v));
            } else {
                spec += "ll";
                spec += conv == 'i' ? 'd' : conv;
                count += std::printf(spec.c_str(), static_cast<long long>(v));
            }
        } else rt_error(c->line, "printf: unsupported conversion %%%c", conv);
    }
    return count;
}

static long call(Node *c) {
    if (c->text == "printf") return do_printf(c);
    Node *f = find_func(prog, c->text);
    if (!f) rt_error(c->line, "undefined function '%s'", c->text.c_str());
    Node *ps = f->kids[1];
    std::vector<Val> args(c->size());
    for (int i = 0; i < c->size(); i++) {
        Node *a = c->kids[i];
        Var *v = a->kind == N_ID ? find_var(a->text) : nullptr;
        if (v && v->val.arr) { args[i] = v->val; args[i].owned = false; }
        else args[i].v = eval(a);
    }
    if (++depth > 3000) rt_error(c->line, "stack overflow (recursion too deep)");
    int old_base = frame_base, mark = sp;
    frame_base = sp;
    for (int i = 0; i < ps->size(); i++) push(ps->kids[i]->text, args[i]);
    retval = 0;
    exec(f->kids[2]);
    long r = retval;
    retval = 0;
    pop_to(mark);
    frame_base = old_base;
    depth--;
    return r;
}

static long eval(Node *n) {
    switch (n->kind) {
    case N_NUM:
    case N_CHAR: return n->ival;
    case N_ID:
    case N_INDEX: return *ref(n);
    case N_BINOP: {
        const std::string &op = n->text;
        if (op == "&&") return eval(n->kids[0]) && eval(n->kids[1]);
        if (op == "||") return eval(n->kids[0]) || eval(n->kids[1]);
        long a = eval(n->kids[0]), b = eval(n->kids[1]);
        return binop(op, a, b, n->line);
    }
    case N_UNOP: {
        long a = eval(n->kids[0]);
        switch (n->text[0]) {
        case '-': return -a;
        case '!': return !a;
        case '~': return ~a;
        default: return a;
        }
    }
    case N_INCDEC: {
        long *p = ref(n->kids[0]);
        long old = *p;
        *p += n->text[0] == '+' ? 1 : -1;
        return n->ival ? *p : old;
    }
    case N_ASSIGN: {
        if (n->text == "=") {
            long r = eval(n->kids[1]);
            *ref(n->kids[0]) = r;
            return r;
        }
        long *p = ref(n->kids[0]);
        long r = eval(n->kids[1]);
        return *p = binop(n->text.substr(0, n->text.size() - 1), *p, r, n->line);
    }
    case N_COND: return eval(n->kids[0]) ? eval(n->kids[1]) : eval(n->kids[2]);
    case N_CALL: return call(n);
    default: rt_error(n->line, "cannot evaluate %s", kind_name(n->kind));
    }
}

static void declare_vars(Node *d) {
    for (int i = 1; i < d->size(); i++) {
        Node *v = d->kids[i];
        Val val;
        if (v->ival >= 0) {
            val.arr = true;
            val.len = v->ival;
            val.owned = true;
            val.a = static_cast<long *>(std::calloc(v->ival ? v->ival : 1, sizeof(long)));
            if (v->size()) for (int j = 0; j < v->kids[0]->size(); j++) val.a[j] = eval(v->kids[0]->kids[j]);
        } else if (v->size()) val.v = eval(v->kids[0]);
        push(v->text, val);
    }
}

static int exec(Node *n) {
    switch (n->kind) {
    case N_BLOCK: {
        int mark = sp, st = ST_NORMAL;
        for (int i = 0; i < n->size() && st == ST_NORMAL; i++) st = exec(n->kids[i]);
        pop_to(mark);
        return st;
    }
    case N_DECLS: declare_vars(n); return ST_NORMAL;
    case N_EXPRSTMT: eval(n->kids[0]); return ST_NORMAL;
    case N_EMPTY: return ST_NORMAL;
    case N_IF:
        if (eval(n->kids[0])) return exec(n->kids[1]);
        return n->size() > 2 ? exec(n->kids[2]) : ST_NORMAL;
    case N_WHILE:
        while (eval(n->kids[0])) {
            int st = exec(n->kids[1]);
            if (st == ST_BREAK) break;
            if (st == ST_RETURN) return st;
        }
        return ST_NORMAL;
    case N_DOWHILE:
        do {
            int st = exec(n->kids[0]);
            if (st == ST_BREAK) break;
            if (st == ST_RETURN) return st;
        } while (eval(n->kids[1]));
        return ST_NORMAL;
    case N_FOR: {
        int mark = sp, res = ST_NORMAL;
        if (n->kids[0]->kind == N_DECLS) declare_vars(n->kids[0]);
        else if (n->kids[0]->kind != N_EMPTY) eval(n->kids[0]);
        while (n->kids[1]->kind == N_EMPTY || eval(n->kids[1])) {
            int st = exec(n->kids[3]);
            if (st == ST_BREAK) break;
            if (st == ST_RETURN) { res = st; break; }
            if (n->kids[2]->kind != N_EMPTY) eval(n->kids[2]);
        }
        pop_to(mark);
        return res;
    }
    case N_RETURN: retval = n->size() ? eval(n->kids[0]) : 0; return ST_RETURN;
    case N_BREAK: return ST_BREAK;
    case N_CONTINUE: return ST_CONTINUE;
    default: rt_error(n->line, "cannot execute %s", kind_name(n->kind));
    }
}

int interpret(Node *root) {
    prog = root;
    sp = 0;
    frame_base = 0;
    nglobals = 0;
    for (Node *e : root->kids)
        if (e->kind == N_DECLS) { declare_vars(e); nglobals = sp; }
    Node *m = find_func(root, "main");
    if (!m) { std::fprintf(stderr, "runtime error: function main() not found\n"); return 2; }
    Node fake;
    fake.kind = N_CALL;
    fake.text = "main";
    fake.line = m->line;
    long r = call(&fake);
    std::fflush(stdout);
    return static_cast<int>(r & 0xff);
}
