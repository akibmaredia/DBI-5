/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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

#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    Name = 258,
    Float = 259,
    Int = 260,
    String = 261,
    CREATE = 262,
    TABLE = 263,
    HEAP = 264,
    INSERT = 265,
    INTO = 266,
    DROP = 267,
    SET = 268,
    OUTPUT = 269,
    STDOUT = 270,
    NONE = 271,
    SELECT = 272,
    GROUP = 273,
    DISTINCT = 274,
    BY = 275,
    FROM = 276,
    WHERE = 277,
    SUM = 278,
    AS = 279,
    AND = 280,
    OR = 281,
    QUIT = 282
  };
#endif
/* Tokens.  */
#define Name 258
#define Float 259
#define Int 260
#define String 261
#define CREATE 262
#define TABLE 263
#define HEAP 264
#define INSERT 265
#define INTO 266
#define DROP 267
#define SET 268
#define OUTPUT 269
#define STDOUT 270
#define NONE 271
#define SELECT 272
#define GROUP 273
#define DISTINCT 274
#define BY 275
#define FROM 276
#define WHERE 277
#define SUM 278
#define AS 279
#define AND 280
#define OR 281
#define QUIT 282

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 34 "Parser.y" /* yacc.c:1909  */

 	struct FuncOperand *myOperand;
	struct FuncOperator *myOperator;
	struct TableList *myTables;
	struct ComparisonOp *myComparison;
	struct Operand *myBoolOperand;
	struct OrList *myOrList;
	struct AndList *myAndList;
	struct NameList *myNames;
	char *actualChars;
	char whichOne;

	struct AttrList *myAttrList;

#line 123 "y.tab.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
