#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include "ast.hpp"
#include "parser.tab.hpp"

extern Node *ast_root;
extern int syntax_errors, lex_errors, yylineno;
void lexer_init(const char *src);
int yylex();
const char *token_name(int tok);

static bool read_file(const std::string &path, std::string &out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::stringstream ss;
    ss << f.rdbuf();
    out = ss.str();
    return true;
}

static int html(const std::string &exe, Node *root) {
    size_t slash = exe.find_last_of('/');
    std::string dir = slash == std::string::npos ? "." : exe.substr(0, slash);
    std::string t, d;
    for (const std::string &w : {dir + "/web", std::string("web")})
        if (read_file(w + "/index.html", t) && read_file(w + "/d3.min.js", d)) break;
    if (t.empty()) return 1;
    for (size_t i = 0; i < t.size();) {
        if (t.compare(i, 6, "__D3__") == 0) { std::fputs(d.c_str(), stdout); i += 6; }
        else if (t.compare(i, 7, "__AST__") == 0) { print_json(root, stdout); i += 7; }
        else std::fputc(t[i++], stdout);
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) { std::fputs("minic tokens|ast|py|html|run file.c\n", stderr); return 1; }
    std::string mode = argv[1], src;
    if (!read_file(argv[2], src)) { std::perror(argv[2]); return 1; }
    lexer_init(src.c_str());

    if (mode == "tokens") {
        int t;
        while ((t = yylex())) {
            std::printf("%d %s", yylineno, token_name(t));
            if (t == ID || t == STRING || t == ASSIGN_OP) std::printf(" %s", yylval.str);
            else if (t == NUM || t == CHARLIT) std::printf(" %ld", yylval.ival);
            std::putchar('\n');
        }
        return lex_errors ? 1 : 0;
    }

    if (yyparse() != 0 || syntax_errors || lex_errors || !ast_root) return 1;
    if (sema_check(ast_root)) return 1;

    if (mode == "ast") print_tree(ast_root, stdout);
    else if (mode == "py") gen_python(ast_root, stdout);
    else if (mode == "html") return html(argv[0], ast_root);
    else if (mode == "run") return interpret(ast_root);
    else return 1;
    return 0;
}
