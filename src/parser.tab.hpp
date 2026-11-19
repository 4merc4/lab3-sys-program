/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_SRC_PARSER_TAB_HPP_INCLUDED
# define YY_YY_SRC_PARSER_TAB_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 1 "src/parser.y"
 struct Node; 

#line 52 "src/parser.tab.hpp"

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    ID = 258,                      /* ID  */
    STRING = 259,                  /* STRING  */
    ASSIGN_OP = 260,               /* ASSIGN_OP  */
    NUM = 261,                     /* NUM  */
    CHARLIT = 262,                 /* CHARLIT  */
    INT = 263,                     /* "int"  */
    VOID = 264,                    /* "void"  */
    IF = 265,                      /* "if"  */
    ELSE = 266,                    /* "else"  */
    WHILE = 267,                   /* "while"  */
    DO = 268,                      /* "do"  */
    FOR = 269,                     /* "for"  */
    RETURN = 270,                  /* "return"  */
    BREAK = 271,                   /* "break"  */
    CONTINUE = 272,                /* "continue"  */
    EQ = 273,                      /* "=="  */
    NE = 274,                      /* "!="  */
    LE = 275,                      /* "<="  */
    GE = 276,                      /* ">="  */
    AND = 277,                     /* "&&"  */
    OR = 278,                      /* "||"  */
    SHL = 279,                     /* "<<"  */
    SHR = 280,                     /* ">>"  */
    INC = 281,                     /* "++"  */
    DEC = 282,                     /* "--"  */
    LOWER_THAN_ELSE = 283,         /* LOWER_THAN_ELSE  */
    UMINUS = 284                   /* UMINUS  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 17 "src/parser.y"
 long ival; char *str; struct Node *node; 

#line 101 "src/parser.tab.hpp"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_SRC_PARSER_TAB_HPP_INCLUDED  */
