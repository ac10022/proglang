#ifndef ASTNODE_H
#define ASTNODE_H

#include "lexer.h"

typedef enum {
    NODE_NULL_EXPR, // Do nothing
    NODE_ADD,       // +
    NODE_SUB,       // -
    NODE_MUL,       // *
    NODE_DIV,       // /
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

    NODE_EXPR_STMT, // Expression statement
    NODE_VARIABLE,  // { variable_type, variable_name }
    NODE_VARIABLE_VALUE,    
    NODE_FUNCTION_DECLARATION,
	NODE_FUNCTION, // { function_name, function_arguments, function_return_type (someday) } 
	NODE_VARAIBLE_DECLARATION,

	NODE_FUNCTION_CALL

} NodeType;

typedef struct ASTNode ASTNode;

typedef struct ASTNode { 
	NodeType node_type;
	Token *token;

	char *symbol_identifier; // function or variable name

	Type variable_type;

	Type function_return_type;

	ASTNode *l_value; 	// x
	ASTNode *r_value;	// 32

	ASTNode *next;
	ASTNode *body;

} ASTNode;

#endif

