#include <cstdio>
#include <string>
#include <vector>
#include "ast.hpp"

static FILE *out;
static int ind;
static long emitted;

struct PSym {
    std::string c;
    std::string py;
};

struct Loop {
    Node *step;
    Node *cond;
};

static std::vector<PSym> syms;
static std::vector<Loop> loops;
static int uniq;

static const char *const PYKW[] = {
    "False", "None", "True", "and", "as", "assert", "async", "await", "break", "class",
    "continue", "def", "del", "elif", "else", "except", "finally", "for", "from", "global", "if",
    "import", "in", "is", "lambda", "nonlocal", "not", "or", "pass", "raise", "return", "try",
    "while", "with", "yield", "sys", nullptr
};

static void emit(const std::string &s) {
    emitted++;
    std::fprintf(out, "%*s%s\n", ind * 4, "", s.c_str());
}

static bool is_reserved(const std::string &n) {
    if (n[0] == '_') return true;
    for (int i = 0; PYKW[i]; i++) if (n == PYKW[i]) return true;
    return false;
}

static bool visible(const std::string &n) {
    for (const PSym &s : syms) if (s.c == n) return true;
    return false;
}

static std::string declare(const std::string &c) {
    std::string py;
    if (is_reserved(c) || visible(c)) py = (c[0] == '_' ? c.substr(1) : c) + "_" + std::to_string(++uniq);
    else py = c;
    if (c[0] == '_' && !visible(c)) py = "v" + c + "_" + std::to_string(++uniq);
    syms.push_back({c, py});
    return py;
}

static std::string pyname(const std::string &c) {
    for (int i = static_cast<int>(syms.size()) - 1; i >= 0; i--)
        if (syms[i].c == c) return syms[i].py;
    return c;
}

static std::string pystr(const std::string &s) {
    std::string r = "\"";
    for (unsigned char ch : s) {
        switch (ch) {
        case '\n': r += "\\n"; break;
        case '\t': r += "\\t"; break;
        case '\r': r += "\\r"; break;
        case '"': r += "\\\""; break;
        case '\\': r += "\\\\"; break;
        default:
            if (ch < 32) {
                char b[8];
                std::snprintf(b, sizeof b, "\\x%02x", ch);
                r += b;
            } else r += static_cast<char>(ch);
        }
    }
    return r + "\"";
}

static std::string ge(Node *n);
static std::string gcond(Node *n);

static bool is_cmp(const std::string &op) {
    return op == "<" || op == ">" || op == "<=" || op == ">=" || op == "==" || op == "!=";
}

static std::string helper_for(const std::string &op) {
    switch (op[0]) {
    case '+': return "_add";
    case '-': return "_sub";
    case '*': return "_mul";
    case '/': return "_div";
    case '%': return "_mod";
    case '&': return "_and";
    case '|': return "_or";
    case '^': return "_xor";
    case '<': return "_shl";
    case '>': return "_shr";
    }
    return "_add";
}

static std::string arith(const std::string &op, const std::string &a, const std::string &b) {
    if (op == "/") return "_div(" + a + ", " + b + ")";
    if (op == "%") return "_mod(" + a + ", " + b + ")";
    return "(" + a + " " + op + " " + b + ")";
}

static std::string base_op(const std::string &assign_op) {
    return assign_op.substr(0, assign_op.size() - 1);
}

static std::string gcond(Node *n) {
    if (n->kind == N_EMPTY) return "True";
    if (n->kind == N_BINOP) {
        const std::string &op = n->text;
        if (is_cmp(op)) return "(" + ge(n->kids[0]) + " " + op + " " + ge(n->kids[1]) + ")";
        if (op == "&&") return "(" + gcond(n->kids[0]) + " and " + gcond(n->kids[1]) + ")";
        if (op == "||") return "(" + gcond(n->kids[0]) + " or " + gcond(n->kids[1]) + ")";
    }
    if (n->kind == N_UNOP && n->text[0] == '!') return "(not " + gcond(n->kids[0]) + ")";
    return ge(n);
}

static std::string ge(Node *n) {
    switch (n->kind) {
    case N_NUM:
    case N_CHAR: return std::to_string(n->ival);
    case N_STR: return pystr(n->text);
    case N_ID: return pyname(n->text);
    case N_INDEX: return pyname(n->text) + "[" + ge(n->kids[0]) + "]";
    case N_BINOP: {
        const std::string &op = n->text;
        if (is_cmp(op) || op == "&&" || op == "||") return "(1 if " + gcond(n) + " else 0)";
        return arith(op, ge(n->kids[0]), ge(n->kids[1]));
    }
    case N_UNOP:
        if (n->text[0] == '!') return "(0 if " + gcond(n->kids[0]) + " else 1)";
        return "(" + n->text + ge(n->kids[0]) + ")";
    case N_INCDEC: {
        Node *lv = n->kids[0];
        std::string d = n->text[0] == '+' ? "1" : "-1";
        if (lv->kind == N_ID) {
            std::string p = pyname(lv->text);
            std::string upd = "(" + p + " := " + p + " + (" + d + "))";
            return n->ival ? upd : "(" + upd + " - (" + d + "))";
        }
        return std::string(n->ival ? "_preinc(" : "_postinc(") + pyname(lv->text) + ", " + ge(lv->kids[0]) + ", " + d + ")";
    }
    case N_ASSIGN: {
        Node *lv = n->kids[0];
        bool simple = n->text == "=";
        std::string rhs = ge(n->kids[1]);
        if (lv->kind == N_ID) {
            std::string p = pyname(lv->text);
            if (simple) return "(" + p + " := " + rhs + ")";
            return "(" + p + " := " + arith(base_op(n->text), p, rhs) + ")";
        }
        std::string arr = pyname(lv->text), idx = ge(lv->kids[0]);
        if (simple) return "_aset(" + arr + ", " + idx + ", " + rhs + ")";
        return "_aop(" + arr + ", " + idx + ", " + helper_for(base_op(n->text)) + ", " + rhs + ")";
    }
    case N_COND: return "(" + ge(n->kids[1]) + " if " + gcond(n->kids[0]) + " else " + ge(n->kids[2]) + ")";
    case N_CALL: {
        std::string args;
        for (int i = 0; i < n->size(); i++) args += (i ? ", " : "") + ge(n->kids[i]);
        if (n->text == "printf") return "_printf(" + args + ")";
        return pyname(n->text) + "(" + args + ")";
    }
    default: return "None";
    }
}

static void gen_stmt(Node *n);

static void gen_body(Node *n) {
    ind++;
    long pos = emitted;
    size_t mark = syms.size();
    if (n->kind == N_BLOCK) for (Node *k : n->kids) gen_stmt(k);
    else gen_stmt(n);
    syms.resize(mark);
    if (emitted == pos) emit("pass");
    ind--;
}

static void gen_decls(Node *d) {
    for (int i = 1; i < d->size(); i++) {
        Node *v = d->kids[i];
        std::string init;
        if (v->ival >= 0) {
            long size = v->ival, k = v->size() ? v->kids[0]->size() : 0;
            std::string items;
            for (long j = 0; j < k; j++) items += (j ? ", " : "") + ge(v->kids[0]->kids[j]);
            if (k == 0) init = "[0] * " + std::to_string(size);
            else if (k == size) init = "[" + items + "]";
            else init = "[" + items + "] + [0] * " + std::to_string(size - k);
        } else init = v->size() ? ge(v->kids[0]) : "0";
        std::string py = declare(v->text);
        emit(py + " = " + init);
    }
}

static std::string target_of(Node *lv) {
    if (lv->kind == N_ID) return pyname(lv->text);
    return pyname(lv->text) + "[" + ge(lv->kids[0]) + "]";
}

static void gen_expr_stmt(Node *e) {
    if (e->kind == N_ASSIGN) {
        Node *lv = e->kids[0];
        std::string rhs = ge(e->kids[1]);
        std::string target = target_of(lv);
        if (e->text == "=") { emit(target + " = " + rhs); return; }
        std::string op = base_op(e->text);
        if (op == "/" || op == "%") {
            if (lv->kind == N_ID) emit(target + " = " + arith(op, target, rhs));
            else emit("_aop(" + pyname(lv->text) + ", " + ge(lv->kids[0]) + ", " + helper_for(op) + ", " + rhs + ")");
        } else emit(target + " " + op + "= " + rhs);
        return;
    }
    if (e->kind == N_INCDEC) {
        emit(target_of(e->kids[0]) + (e->text[0] == '+' ? " += 1" : " -= 1"));
        return;
    }
    emit(ge(e));
}

static void gen_stmt(Node *n) {
    switch (n->kind) {
    case N_BLOCK: {
        size_t m = syms.size();
        for (Node *k : n->kids) gen_stmt(k);
        syms.resize(m);
        break;
    }
    case N_DECLS: gen_decls(n); break;
    case N_EXPRSTMT: gen_expr_stmt(n->kids[0]); break;
    case N_IF: {
        emit("if " + gcond(n->kids[0]) + ":");
        gen_body(n->kids[1]);
        Node *e = n->size() > 2 ? n->kids[2] : nullptr;
        while (e && e->kind == N_IF) {
            emit("elif " + gcond(e->kids[0]) + ":");
            gen_body(e->kids[1]);
            e = e->size() > 2 ? e->kids[2] : nullptr;
        }
        if (e) { emit("else:"); gen_body(e); }
        break;
    }
    case N_WHILE:
        emit("while " + gcond(n->kids[0]) + ":");
        loops.push_back({nullptr, nullptr});
        gen_body(n->kids[1]);
        loops.pop_back();
        break;
    case N_DOWHILE:
        emit("while True:");
        loops.push_back({nullptr, n->kids[1]});
        gen_body(n->kids[0]);
        ind++;
        emit("if not " + gcond(n->kids[1]) + ":");
        ind++;
        emit("break");
        ind -= 2;
        loops.pop_back();
        break;
    case N_FOR: {
        size_t m = syms.size();
        if (n->kids[0]->kind == N_DECLS) gen_decls(n->kids[0]);
        else if (n->kids[0]->kind != N_EMPTY) gen_expr_stmt(n->kids[0]);
        emit("while " + gcond(n->kids[1]) + ":");
        loops.push_back({n->kids[2]->kind == N_EMPTY ? nullptr : n->kids[2], nullptr});
        gen_body(n->kids[3]);
        if (n->kids[2]->kind != N_EMPTY) { ind++; gen_expr_stmt(n->kids[2]); ind--; }
        loops.pop_back();
        syms.resize(m);
        break;
    }
    case N_RETURN: emit(n->size() ? "return " + ge(n->kids[0]) : "return"); break;
    case N_BREAK: emit("break"); break;
    case N_CONTINUE: {
        Loop &l = loops.back();
        if (l.step) gen_expr_stmt(l.step);
        if (l.cond) {
            emit("if not " + gcond(l.cond) + ":");
            ind++;
            emit("break");
            ind--;
        }
        emit("continue");
        break;
    }
    default: break;
    }
}

static const char *const RUNTIME =
    "import sys\n\n"
    "def _div(a, b):\n    q = abs(a) // abs(b)\n    return q if (a < 0) == (b < 0) else -q\n\n"
    "def _mod(a, b):\n    return a - _div(a, b) * b\n\n"
    "def _add(a, b): return a + b\n"
    "def _sub(a, b): return a - b\n"
    "def _mul(a, b): return a * b\n"
    "def _and(a, b): return a & b\n"
    "def _or(a, b): return a | b\n"
    "def _xor(a, b): return a ^ b\n"
    "def _shl(a, b): return a << b\n"
    "def _shr(a, b): return a >> b\n\n"
    "def _aset(arr, i, v):\n    arr[i] = v\n    return v\n\n"
    "def _aop(arr, i, f, v):\n    arr[i] = f(arr[i], v)\n    return arr[i]\n\n"
    "def _preinc(arr, i, d):\n    arr[i] += d\n    return arr[i]\n\n"
    "def _postinc(arr, i, d):\n    arr[i] += d\n    return arr[i] - d\n\n"
    "def _printf(fmt, *args):\n    s = fmt % args\n    sys.stdout.write(s)\n    return len(s)\n\n";

void gen_python(Node *root, FILE *o) {
    out = o;
    ind = 0;
    emitted = 0;
    uniq = 0;
    syms.clear();
    loops.clear();
    std::fputs(RUNTIME, out);
    std::string globals;
    for (Node *e : root->kids) {
        if (e->kind != N_DECLS) continue;
        gen_decls(e);
        for (int k = 1; k < e->size(); k++)
            if (e->kids[k]->ival < 0) globals += (globals.empty() ? "" : ", ") + pyname(e->kids[k]->text);
    }
    for (Node *f : root->kids) {
        if (f->kind != N_FUNC) continue;
        std::string py = is_reserved(f->text) ? (f->text[0] == '_' ? f->text.substr(1) : f->text) + "_f" : f->text;
        syms.push_back({f->text, py});
    }
    size_t gmark = syms.size();
    for (Node *f : root->kids) {
        if (f->kind != N_FUNC) continue;
        std::fputc('\n', out);
        std::string params;
        Node *ps = f->kids[1];
        for (int k = 0; k < ps->size(); k++) params += (k ? ", " : "") + declare(ps->kids[k]->text);
        emit("def " + pyname(f->text) + "(" + params + "):");
        if (!globals.empty()) {
            ind++;
            emit("global " + globals);
            ind--;
        }
        gen_body(f->kids[2]);
        syms.resize(gmark);
    }
    std::fputs("\n\nif __name__ == \"__main__\":\n    sys.exit(main() or 0)\n", out);
}
