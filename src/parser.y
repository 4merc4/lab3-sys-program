%code requires { struct Node; }
%{
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "ast.hpp"

int yylex(void);
void yyerror(const char *msg);
extern int yylineno;
Node *ast_root = nullptr;
int syntax_errors = 0;
static Node *type_node(const char *t) { return mk(N_TYPE, t, 0, yylineno); }
%}

%define parse.error verbose
%union { long ival; char *str; struct Node *node; }

%token <str>  ID STRING ASSIGN_OP
%token <ival> NUM CHARLIT
%token INT "int" VOID "void" IF "if" ELSE "else" WHILE "while" DO "do" FOR "for"
%token RETURN "return" BREAK "break" CONTINUE "continue"
%token EQ "==" NE "!=" LE "<=" GE ">=" AND "&&" OR "||" SHL "<<" SHR ">>" INC "++" DEC "--"

%precedence LOWER_THAN_ELSE
%precedence ELSE
%right '=' ASSIGN_OP
%right '?' ':'
%left OR
%left AND
%left '|'
%left '^'
%left '&'
%left EQ NE
%left '<' '>' LE GE
%left SHL SHR
%left '+' '-'
%left '*' '/' '%'
%right UMINUS '!' '~' INC DEC

%type <node> program external function decl type declarators declarator initlist
%type <node> params param_list param block items item stmt expr lvalue opt_expr for_init args arg_list

%start program
%%

program
    : external                  { ast_root = $$ = add(mk(N_PROGRAM, "", 0, 1), $1); }
    | program external          { $$ = add($1, $2); }
    ;

external
    : function
    | decl
    ;

type
    : INT                       { $$ = type_node("int"); }
    | VOID                      { $$ = type_node("void"); }
    ;

function
    : type ID '(' params ')' block
                                { $$ = mk3(N_FUNC, $2, 0, $1->line, $1, $4, $6); }
    ;

params
    : %empty { $$ = mk(N_PARAMS, "", 0, yylineno); }
    | VOID                      { $$ = mk(N_PARAMS, "", 0, yylineno); }
    | param_list
    ;
param_list
    : param                     { $$ = add(mk(N_PARAMS, "", 0, yylineno), $1); }
    | param_list ',' param      { $$ = add($1, $3); }
    ;
param
    : INT ID                    { $$ = mk(N_PARAM, $2, 0, yylineno); }
    | INT ID '[' ']'            { $$ = mk(N_PARAM, $2, 1, yylineno); }
    ;

decl
    : type declarators ';'      { $$ = mk1(N_DECLS, "", 0, $1->line, $1);
                                  for (int i = 0; i < $2->size(); i++) add($$, $2->kids[i]); }
    ;
declarators
    : declarator                { $$ = add(mk(N_DECLS, "", 0, yylineno), $1); }
    | declarators ',' declarator{ $$ = add($1, $3); }
    ;
declarator
    : ID                        { $$ = mk(N_VARDECL, $1, -1, yylineno); }
    | ID '=' expr               { $$ = mk1(N_VARDECL, $1, -1, yylineno, $3); }
    | ID '[' NUM ']'            { $$ = mk(N_VARDECL, $1, $3, yylineno); }
    | ID '[' NUM ']' '=' '{' initlist '}'
                                { $$ = mk1(N_VARDECL, $1, $3, yylineno, $7); }
    | ID '[' ']' '=' '{' initlist '}'
                                { $$ = mk1(N_VARDECL, $1, 0, yylineno, $6); }
    ;
initlist
    : expr                      { $$ = add(mk(N_INITLIST, "", 0, yylineno), $1); }
    | initlist ',' expr         { $$ = add($1, $3); }
    ;

block
    : '{' '}'                   { $$ = mk(N_BLOCK, "", 0, yylineno); }
    | '{' items '}'             { $$ = $2; }
    ;
items
    : item                      { $$ = add(mk(N_BLOCK, "", 0, yylineno), $1); }
    | items item                { $$ = add($1, $2); }
    ;
item
    : decl
    | stmt
    ;

stmt
    : block
    | expr ';'                  { $$ = mk1(N_EXPRSTMT, "", 0, $1->line, $1); }
    | ';'                       { $$ = mk(N_EMPTY, "", 0, yylineno); }
    | IF '(' expr ')' stmt %prec LOWER_THAN_ELSE
                                { $$ = mk2(N_IF, "", 0, $3->line, $3, $5); }
    | IF '(' expr ')' stmt ELSE stmt
                                { $$ = mk3(N_IF, "", 0, $3->line, $3, $5, $7); }
    | WHILE '(' expr ')' stmt   { $$ = mk2(N_WHILE, "", 0, $3->line, $3, $5); }
    | DO stmt WHILE '(' expr ')' ';'
                                { $$ = mk2(N_DOWHILE, "", 0, $5->line, $2, $5); }
    | FOR '(' for_init ';' opt_expr ';' opt_expr ')' stmt
                                { Node *f = mk(N_FOR, "", 0, $3->line);
                                  add(f, $3); add(f, $5); add(f, $7); add(f, $9); $$ = f; }
    | RETURN ';'                { $$ = mk(N_RETURN, "", 0, yylineno); }
    | RETURN expr ';'           { $$ = mk1(N_RETURN, "", 0, $2->line, $2); }
    | BREAK ';'                 { $$ = mk(N_BREAK, "", 0, yylineno); }
    | CONTINUE ';'              { $$ = mk(N_CONTINUE, "", 0, yylineno); }
    | error ';'                 { $$ = mk(N_EMPTY, "", 0, yylineno); yyerrok; }
    ;

for_init
    : %empty { $$ = mk(N_EMPTY, "", 0, yylineno); }
    | expr
    | type declarators          { $$ = mk1(N_DECLS, "", 0, $1->line, $1);
                                  for (int i = 0; i < $2->size(); i++) add($$, $2->kids[i]); }
    ;
opt_expr
    : %empty { $$ = mk(N_EMPTY, "", 0, yylineno); }
    | expr
    ;

lvalue
    : ID                        { $$ = mk(N_ID, $1, 0, yylineno); }
    | ID '[' expr ']'           { $$ = mk1(N_INDEX, $1, 0, yylineno, $3); }
    ;

expr
    : NUM                       { $$ = mk(N_NUM, "", $1, yylineno); }
    | CHARLIT                   { $$ = mk(N_CHAR, "", $1, yylineno); }
    | STRING                    { $$ = mk(N_STR, $1, 0, yylineno); }
    | lvalue
    | ID '(' args ')'           { $$ = $3; $$->text = $1; }
    | '(' expr ')'              { $$ = $2; }
    | lvalue '=' expr           { $$ = mk2(N_ASSIGN, "=", 0, $1->line, $1, $3); }
    | lvalue ASSIGN_OP expr %prec '='
                                { $$ = mk2(N_ASSIGN, $2, 0, $1->line, $1, $3); }
    | expr '?' expr ':' expr    { $$ = mk3(N_COND, "", 0, $1->line, $1, $3, $5); }
    | expr OR expr              { $$ = mk2(N_BINOP, "||", 0, $1->line, $1, $3); }
    | expr AND expr             { $$ = mk2(N_BINOP, "&&", 0, $1->line, $1, $3); }
    | expr '|' expr             { $$ = mk2(N_BINOP, "|",  0, $1->line, $1, $3); }
    | expr '^' expr             { $$ = mk2(N_BINOP, "^",  0, $1->line, $1, $3); }
    | expr '&' expr             { $$ = mk2(N_BINOP, "&",  0, $1->line, $1, $3); }
    | expr EQ expr              { $$ = mk2(N_BINOP, "==", 0, $1->line, $1, $3); }
    | expr NE expr              { $$ = mk2(N_BINOP, "!=", 0, $1->line, $1, $3); }
    | expr '<' expr             { $$ = mk2(N_BINOP, "<",  0, $1->line, $1, $3); }
    | expr '>' expr             { $$ = mk2(N_BINOP, ">",  0, $1->line, $1, $3); }
    | expr LE expr              { $$ = mk2(N_BINOP, "<=", 0, $1->line, $1, $3); }
    | expr GE expr              { $$ = mk2(N_BINOP, ">=", 0, $1->line, $1, $3); }
    | expr SHL expr             { $$ = mk2(N_BINOP, "<<", 0, $1->line, $1, $3); }
    | expr SHR expr             { $$ = mk2(N_BINOP, ">>", 0, $1->line, $1, $3); }
    | expr '+' expr             { $$ = mk2(N_BINOP, "+",  0, $1->line, $1, $3); }
    | expr '-' expr             { $$ = mk2(N_BINOP, "-",  0, $1->line, $1, $3); }
    | expr '*' expr             { $$ = mk2(N_BINOP, "*",  0, $1->line, $1, $3); }
    | expr '/' expr             { $$ = mk2(N_BINOP, "/",  0, $1->line, $1, $3); }
    | expr '%' expr             { $$ = mk2(N_BINOP, "%",  0, $1->line, $1, $3); }
    | '-' expr %prec UMINUS     { $$ = mk1(N_UNOP, "-", 0, $2->line, $2); }
    | '+' expr %prec UMINUS     { $$ = mk1(N_UNOP, "+", 0, $2->line, $2); }
    | '!' expr                  { $$ = mk1(N_UNOP, "!", 0, $2->line, $2); }
    | '~' expr                  { $$ = mk1(N_UNOP, "~", 0, $2->line, $2); }
    | INC lvalue                { $$ = mk1(N_INCDEC, "++", 1, $2->line, $2); }
    | DEC lvalue                { $$ = mk1(N_INCDEC, "--", 1, $2->line, $2); }
    | lvalue INC                { $$ = mk1(N_INCDEC, "++", 0, $1->line, $1); }
    | lvalue DEC                { $$ = mk1(N_INCDEC, "--", 0, $1->line, $1); }
    ;

args
    : %empty { $$ = mk(N_CALL, "", 0, yylineno); }
    | arg_list
    ;
arg_list
    : expr                      { $$ = add(mk(N_CALL, "", 0, $1->line), $1); }
    | arg_list ',' expr         { $$ = add($1, $3); }
    ;

%%

void yyerror(const char *msg) {
    fprintf(stderr, "syntax error at line %d: %s\n", yylineno, msg);
    syntax_errors++;
}

const char *token_name(int tok) { return yysymbol_name(YYTRANSLATE(tok)); }
