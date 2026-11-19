#pragma once
#include <cstdio>
#include <string>
#include <vector>

enum NodeKind {
    N_PROGRAM, N_FUNC, N_TYPE, N_PARAMS, N_PARAM, N_DECLS, N_VARDECL, N_INITLIST,
    N_BLOCK, N_EXPRSTMT, N_EMPTY, N_IF, N_WHILE, N_DOWHILE, N_FOR,
    N_RETURN, N_BREAK, N_CONTINUE,
    N_NUM, N_CHAR, N_STR, N_ID, N_BINOP, N_UNOP, N_INCDEC, N_ASSIGN, N_COND,
    N_INDEX, N_CALL
};

struct Node {
    NodeKind kind;
    std::string text;
    long ival = 0;
    int line = 0;
    std::vector<Node *> kids;
    int size() const { return static_cast<int>(kids.size()); }
};

Node *mk(NodeKind k, const std::string &text, long ival, int line);
Node *add(Node *parent, Node *child);
Node *mk1(NodeKind k, const std::string &t, long iv, int line, Node *a);
Node *mk2(NodeKind k, const std::string &t, long iv, int line, Node *a, Node *b);
Node *mk3(NodeKind k, const std::string &t, long iv, int line, Node *a, Node *b, Node *c);
const char *kind_name(NodeKind k);
std::string node_label(const Node *n);
void print_tree(const Node *n, FILE *f);
void print_json(const Node *n, FILE *f);
void json_string(FILE *f, const std::string &s);

int sema_check(Node *root);
Node *find_func(Node *root, const std::string &name);
int interpret(Node *root);
void gen_python(Node *root, FILE *out);
