#ifndef PARSER_H
#define PARSER_H
#include <string.h>
#include "lexer.h"
#include "ASTNode.h"

ASTNode *generate_ast(Token *head);

ASTNode *parse_variable_assignment(Token **token);
ASTNode *parse_variable_declaration(Token **token);
ASTNode *parse_statement	(Token **token);
ASTNode *parse_function 	(Token **token);
ASTNode *parse_if_statement	(Token **token);
ASTNode *parse_while_statement(Token **token);
ASTNode *parse_for_statement(Token **token);

ASTNode *new_node_binary(NodeType type, ASTNode *l_value, ASTNode *r_value);
ASTNode *new_node_unary(NodeType type, ASTNode *unary_operand);
ASTNode *new_node_memidentifier(ASTNode *l_value, char *identifier);

ASTNode *parse_expression(Token **token);
ASTNode *parse_variable_assignment(Token **token);
ASTNode *parse_logical_or(Token **token);
ASTNode *parse_logical_and(Token **token);
ASTNode *parse_equality(Token **token);
ASTNode *parse_comparison(Token **token);
ASTNode *parse_term(Token **token);
ASTNode *parse_factor(Token **token);
ASTNode *parse_exponentiation(Token **token);
ASTNode *parse_unary(Token **token);
ASTNode *parse_postfix(Token **token);
ASTNode *parse_function_call(Token **token);
ASTNode *parse_else(Token **token);

bool is_token_type(Token *token, TokenType type);

#ifdef DEBUG
const char* node_to_str(NodeType type);
void trace(ASTNode* head, size_t depth);
#endif

#endif
