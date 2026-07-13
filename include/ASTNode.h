#ifndef ASTNODE_H
#define ASTNODE_H

#include "lexer.h"
#include "parser.h"

typedef enum {
    NODE_NULL_EXPR, // Do nothing (luca: im assuming this is for literal NULL values?)
    NODE_ADD,       // +
    NODE_INCREMENT, // ++
    NODE_SUB,       // -
    NODE_DECREMENT, // --
    NODE_MUL,       // *
    NODE_DIV,       // /
    NODE_EXP,       // **
    NODE_NEG,       // unary -
    NODE_MOD,       // %
    NODE_BITAND,    // &
    NODE_BITOR,     // |
    NODE_BITXOR,    // ^
    NODE_SHL,       // <<
    NODE_SHR,       // >>
    NODE_EQ,        // ==
    NODE_NE,        // !=
    NODE_LT,        // <
    NODE_LE,        // <=
    NODE_GT,        // >
    NODE_GE,        // >=
    NODE_ASSIGN,    // =
    NODE_COND,      // ?:
    NODE_COMMA,     // ,
    NODE_MEMBER,    // . (struct member access)
    NODE_ADDR,      // unary &
    NODE_DEREF,     // unary *
    NODE_NOT,       // !
    NODE_BITNOT,    // ~
    NODE_LOGAND,    // &&
    NODE_LOGOR,     // ||

    NODE_RETURN,    // "return"
    NODE_IF,        // "if"
    NODE_FOR,       // "for" or "while"
    NODE_SWITCH,    // "switch"
    NODE_CASE,      // "case"

    NODE_BLOCK,     // { ... }
    NODE_INDEX,     // /* array */[ ... ]

    NODE_EXPR_STMT, // Expression statement
    NODE_VARIABLE,  // { variable_type, variable_name }
    NODE_VARIABLE_VALUE,    
    NODE_FUNCTION_DECLARATION,
	NODE_FUNCTION, // { function_name, function_arguments, function_return_type (someday) } 
	NODE_VARAIBLE_DECLARATION,

    NODE_LITERAL_INT,
    NODE_LITERAL_FLOAT,
    NODE_LITERAL_STRING,

	NODE_FUNCTION_CALL

} NodeType;

typedef struct ASTNode ASTNode;

typedef struct ASTNode { 
	NodeType node_type;
	Token *token;

	Symbol *variable_symbol;

	TypeInfo *variable_typeinfo;
	TypeInfo *function_return_type;

	ASTNode *l_value; 	// x
	ASTNode *r_value;	// 32

	ASTNode *next;
	ASTNode *body;

    // if statements
    ASTNode *condition;
    ASTNode *on_condition_success;
    ASTNode *on_condition_failure;
} ASTNode;

#endif

