#ifndef PARSER_H
#define PARSER_H
#include <string.h>
#include "lexer.h"
#include "ASTNode.h"

ASTNode *generate_ast(Token *head);

ASTNode *parse_variable_assignment(Token **token);
ASTNode *parse_variable_declaration(Token **token);
ASTNode *parse_expression	(Token **token);
ASTNode *parse_statement	(Token **token);
ASTNode *parse_function 	(Token **token);
ASTNode *parse_if_statement	(Token **token);
ASTNode *parse_while_statement(Token **token);
ASTNode *parse_for_statement(Token **token);

bool is_token_type(Token *token, TokenType type);

#endif
