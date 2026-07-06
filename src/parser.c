#include "../include/parser.h"
#include <stdio.h>
#include <stdlib.h>

ASTNode *generate_ast(Token *head) {
	Token *current_token = head;
	Token **current = &current_token;

	ASTNode *root;

	while ( (*current)->token_type != TOKEN_EOF ) {
		printf("%d\n", (*current)->token_type);
		parse_statement(current);
	}

	return root;
}

ASTNode *parse_variable_assignment(Token **token) {
	
}

ASTNode *parse_variable_declaration(Token **token) {
    Type type_specifier = (*token)->typeinfo->type; // i32

    *token = (*token)->next; // must be TOKEN_SYMBOL_IDENTIFIER
	if ((*token)->token_type != TOKEN_SYMBOL_IDENTIFIER) {
		fprintf(stderr, "Syntax Error at line %lu: Expected ';'\n", (*token)->line_number);
		exit(1);
	}

	ASTNode *declaration_node = malloc(sizeof(ASTNode));
	declaration_node->node_type = NODE_VARAIBLE_DECLARATION; // points to variable identifer and variable's value (if assigned)

    ASTNode *variable_node = malloc(sizeof(ASTNode));
    variable_node->node_type = NODE_VARIABLE; // contains variable name
    variable_node->symbol_identifier = (*token)->lexeme;
	variable_node->variable_type = (*token)->typeinfo->type;

	declaration_node->l_value = variable_node;

    *token = (*token)->next; // ";" or "="
    if ((*token)->token_type == TOKEN_PUNCTUATOR) {
		if ((*token)->punc_type == PUNC_ASSIGNMENT) {
			*token = (*token)->next;   

			ASTNode *value_node = parse_expression(token);
			
			if ((*token)->punc_type != PUNC_SEMICOLON) {
				fprintf(stderr, "Syntax Error: Expected ';'\n");
				exit(1);
			}

			declaration_node->r_value = value_node;

			return declaration_node;
		} else if ((*token)->punc_type == PUNC_SEMICOLON) {
			return declaration_node;
		}
    } else {
        fprintf(stderr, "Syntax Error at line %lu: Expected ';'\n", (*token)->line_number);
        exit(1);
	}
    
    return variable_node;
}

ASTNode *parse_expression(Token **token) { 
	return NULL;
}

ASTNode *parse_function(Token **token) {

}

ASTNode *parse_statement(Token **token) { 
	switch ((*token)->token_type) {
		case TOKEN_KEYWORD_FUNCTION:
			parse_function(token);
			break;
		case TOKEN_PRIMITIVE_TYPE_SPECIFIER:
			parse_variable_assignment(token);
			break;
		case TOKEN_KEYWORD_IF:
			parse_if_statement(token);
			break;
		case TOKEN_KEYWORD_WHILE:
			parse_while_statement(token);
			break;
		case TOKEN_KEYWORD_FOR:
			parse_for_statement(token);
			break;
		default:
			fprintf(stderr, "Syntax Error: Unknown statement token\n");
            exit(1);
	}
}

bool equal(Token *token, char *op) {
  return memcmp(token->location, op, token->length) == 0 && op[token->length] == '\0';
}

bool is_token_type(Token *token, TokenType token_type) {
	if (token == NULL) {
		return false;
	}

	if (token->token_type == token_type) {
		return true;
	}

	return false;
}

ASTNode *parse_if_statement	(Token **token) {

}
ASTNode *parse_while_statement(Token **token) {

}
ASTNode *parse_for_statement(Token **token) {

}
