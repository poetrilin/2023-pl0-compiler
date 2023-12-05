#include <stdio.h>

#define NRW        11     // number of reserved words
#define TXMAX      500    // length of identifier table
#define MAXNUMLEN  14     // maximum number of digits in numbers
#define NSYM       10     // maximum number of symbols in array ssym and csym
#define MAXIDLEN   10     // length of identifiers

#define MAXADDRESS 32767  // maximum address
#define MAXLEVEL   32     // maximum depth of nesting block
#define CXMAX      500    // size of code array

#define MAXSYM     30     // maximum number of symbols  

#define STACKSIZE  1000   // maximum storage

enum symtype
{
    SYM_NULL,       // 空符号
    SYM_IDENTIFIER, // 标识符
    SYM_NUMBER,     // 数字
    SYM_PLUS,       // 加号
    SYM_MINUS,      // 减号
    SYM_TIMES,      // 乘号
    SYM_SLASH,      // 除号
    SYM_ODD,        // 奇数判断
    SYM_EQU,        // 等于
    SYM_NEQ,        // 不等于
    SYM_LES,        // 小于
    SYM_LEQ,        // 小于等于
    SYM_GTR,        // 大于
    SYM_GEQ,        // 大于等于
    SYM_LPAREN,     // 左括号
    SYM_RPAREN,     // 右括号
    SYM_COMMA,      // 逗号
    SYM_SEMICOLON,  // 分号
    SYM_PERIOD,     // 句号
    SYM_BECOMES,    // 赋值符号 :=
    SYM_BEGIN,      // 开始符号 begin
    SYM_END,        // 结束符号 end
    SYM_IF,         // 条件语句符号 if
    SYM_THEN,       // 条件语句中的 "then"
    SYM_WHILE,      // 循环语句符号 while
    SYM_DO,         // 循环语句中的 "do"
    SYM_CALL,       // 过程调用符号 call
    SYM_CONST,      // 常量声明符号 const
    SYM_VAR,        // 变量声明符号 var
    SYM_PROCEDURE   // 过程声明符号 procedure
};

// 枚举类型 idtype 定义了标识符的类型，包括常量、变量和过程
enum idtype
{
    ID_CONSTANT,   // 常量标识符
    ID_VARIABLE,   // 变量标识符
    ID_PROCEDURE   // 过程标识符
};

// 枚举类型 opcode 定义了指令的操作码
enum opcode
{
    LIT,  // 将常量放入栈顶
    OPR,  // 执行运算或逻辑操作
    LOD,  // 将变量放入栈顶
    STO,  // 将栈顶内容存入变量
    CAL,  // 调用过程
    INT,  // 初始化栈顶指针
    JMP,  // 无条件跳转
    JPC   // 条件跳转
};

// 枚举类型 oprcode 定义了运算符的操作码
enum oprcode
{
    OPR_RET,  // 返回
    OPR_NEG,  // 取反
    OPR_ADD,  // 加法
    OPR_MIN,  // 减法
    OPR_MUL,  // 乘法
    OPR_DIV,  // 除法
    OPR_ODD,  // 判断奇偶
    OPR_EQU,  // 等于
    OPR_NEQ,  // 不等于
    OPR_LES,  // 小于
    OPR_LEQ,  // 小于等于
    OPR_GTR,  // 大于
    OPR_GEQ   // 大于等于
};

// instruction 结构体定义了指令的格式，包括操作码和操作数
typedef struct
{
    int f; // function code
    int l; // level
    int a; // displacement address
} instruction;
//////////////////////////////////////////////////////////////////////
char* err_msg[] =
{
/*  0 */    "",
/*  1 */    "Found ':=' when expecting '='.",
/*  2 */    "There must be a number to follow '='.",
/*  3 */    "There must be an '=' to follow the identifier.",
/*  4 */    "There must be an identifier to follow 'const', 'var', or 'procedure'.",
/*  5 */    "Missing ',' or ';'.",
/*  6 */    "Incorrect procedure name.",
/*  7 */    "Statement expected.",
/*  8 */    "Follow the statement is an incorrect symbol.",
/*  9 */    "'.' expected.",
/* 10 */    "';' expected.",
/* 11 */    "Undeclared identifier.",
/* 12 */    "Illegal assignment.",
/* 13 */    "':=' expected.",
/* 14 */    "There must be an identifier to follow the 'call'.",
/* 15 */    "A constant or variable can not be called.",
/* 16 */    "'then' expected.",
/* 17 */    "';' or 'end' expected.",
/* 18 */    "'do' expected.",
/* 19 */    "Incorrect symbol.",
/* 20 */    "Relative operators expected.",
/* 21 */    "Procedure identifier can not be in an expression.",
/* 22 */    "Missing ')'.",
/* 23 */    "The symbol can not be followed by a factor.",
/* 24 */    "The symbol can not be as the beginning of an expression.",
/* 25 */    "The number is too great.",
/* 26 */    "",
/* 27 */    "",
/* 28 */    "",
/* 29 */    "",
/* 30 */    "",
/* 31 */    "",
/* 32 */    "There are too many levels."
};

//////////////////////////////////////////////////////////////////////
char ch;         // last character read
int  sym;        // last symbol read
char id[MAXIDLEN + 1]; // last identifier read
int  num;        // last number read
int  cc;         // character count
int  ll;         // line length
int  kk;
int  err;
int  cx;         // index of current instruction to be generated.
int  level = 0;
int  tx = 0;

char line[80];

instruction code[CXMAX];

/**
 * @brief 保留字字符串数组，存储 PL/0 编译器中的保留字
 */
char* word[NRW + 1] =
{
	"", /* place holder */
	"begin", "call", "const", "do", "end","if",
	"odd", "procedure", "then", "var", "while"
};

/**
 * @brief 与保留字对应的符号集合数组
 */
int wsym[NRW + 1] =
{
	SYM_NULL, SYM_BEGIN, SYM_CALL, SYM_CONST, SYM_DO, SYM_END,
	SYM_IF, SYM_ODD, SYM_PROCEDURE, SYM_THEN, SYM_VAR, SYM_WHILE
};

/**
 * @brief 与特殊符号对应的符号集合数组
 */
int ssym[NSYM + 1] =
{
	SYM_NULL, SYM_PLUS, SYM_MINUS, SYM_TIMES, SYM_SLASH,
	SYM_LPAREN, SYM_RPAREN, SYM_EQU, SYM_COMMA, SYM_PERIOD, SYM_SEMICOLON
};

/**
 * @brief 特殊符号的字符表示数组
 */
char csym[NSYM + 1] =
{
	' ', '+', '-', '*', '/', '(', ')', '=', ',', '.', ';'
};

/**
 * @brief 虚拟机指令助记符字符串数组
 */
#define MAXINS   8
char* mnemonic[MAXINS] =
{
	"LIT", "OPR", "LOD", "STO", "CAL", "INT", "JMP", "JPC"
};

/**
 * @brief 编译表的条目结构
 */
typedef struct
{
	char name[MAXIDLEN + 1]; /**< 标识符名字 */
	int  kind; /**< 标识符类型（常量、变量、过程） */
	int  value; /**< 标识符的值 */
} comtab;

/**
 * @brief 编译表数组，存储 PL/0 编译器的编译表
 */
comtab table[TXMAX];

/**
 * @brief 符号表的条目结构
 */
typedef struct
{
	char  name[MAXIDLEN + 1]; /**< 标识符名字 */
	int   kind; /**< 标识符类型（常量、变量、过程） */
	short level; /**< 标识符所在层次 */
	short address; /**< 标识符地址 */
} mask;

FILE* infile;

// EOF PL0.h
