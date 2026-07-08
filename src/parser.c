#include "../include/parser.h"
#include "../include/base.h"

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
	printf("EOF\n");

	return root;
}

ASTNode *parse_variable_declaration(Token **token) {
	printf("Parsing variable declaration\n");
	assert((*token)->typeinfo != NULL); // if this calls its probably because its a type we havent provided support in the lexer yet

    Type type_specifier = (*token)->typeinfo->type; // i32

    *token = (*token)->next; // must be TOKEN_SYMBOL_IDENTIFIER
	if ((*token)->token_type != TOKEN_SYMBOL_IDENTIFIER) {
		ERR_SYNTAX(*token, /* expected a */ "variable identifer");
	}

	ASTNode *declaration_node = malloc(sizeof(ASTNode));
	declaration_node->node_type = NODE_VARAIBLE_DECLARATION; // points to variable identifer and variable's value (if assigned)

    ASTNode *variable_node = malloc(sizeof(ASTNode));
    variable_node->node_type = NODE_VARIABLE; // contains variable name
    variable_node->symbol_identifier = (*token)->lexeme;
	variable_node->variable_type = type_specifier; // this was causing segfault earlier, its because we've advanced the token, but we store the typeinfo before anyway so we just use this

	declaration_node->l_value = variable_node;

    *token = (*token)->next; // ";" or "="
    if ((*token)->token_type == TOKEN_PUNCTUATOR) {
		if ((*token)->punc_type == PUNC_ASSIGNMENT) {
			*token = (*token)->next;   

			ASTNode *value_node = parse_expression(token);
			
			if ((*token)->punc_type != PUNC_SEMICOLON) {
				ERR_SYNTAX(*token, /*expected a */ "';'");
			}

			declaration_node->r_value = value_node;

			*token = (*token)->next;
			return declaration_node;
		} else if ((*token)->punc_type == PUNC_SEMICOLON) {
			*token = (*token)->next;
			return declaration_node;
		}
    } else {
		ERR_SYNTAX(*token, /* expected a */ "';'");
	}
    
    return variable_node;
}

ASTNode *parse_function(Token **token) {

}

ASTNode *parse_statement(Token **token) { 
	printf("parsing statement\n");
	printf("%s\n", token_type_to_str((*token)));
	switch ((*token)->token_type) {
		case TOKEN_KEYWORD_FUNCTION:
			return parse_function(token);
		case TOKEN_PRIMITIVE_TYPE_SPECIFIER:
			printf("hit?\n");
			return parse_variable_declaration(token);
		case TOKEN_KEYWORD_IF:
			return parse_if_statement(token);
		case TOKEN_KEYWORD_WHILE:
			return parse_while_statement(token);
		case TOKEN_KEYWORD_FOR:
			return parse_for_statement(token);
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

// operator presedence (lowest to highest)

// name                                 symbols                 examples
// Assignment	                        =, +=, -=, *=, /=	    x = y + 2
// Logical OR	                        ||	                    a || b
// Logical AND	                        &&	                    a && b
// Equality	                            ==, !=	                status == 200
// Comparison	                        <, >, <=, >=	        age >= 18
// Term (Additive)	                    +, -	                income - tax
// Factor (Multiplicative)	            *, /, %	                hours * wage
// Exponentiation						**						2 ** 5
// Unary / Prefix	                    !, ++, --	            !is_active, count--
// Postfix / Call	                    (), [], ., ->	        matrix[i][j]
// Identifiers, Literals, Grouping	                            x, 42, "hello", (a + b)

// i was looking up shunting yard, but apparently for an AST the easiest way to parse expressions is descent parsing

ASTNode *parse_expression(Token **token) { 
	printf("parsing experssion\n");
	// while ((*token)->punc_type != PUNC_SEMICOLON) {
	// 	(*token) = (*token)->next;
	// }
	// return NULL;
	return parse_variable_assignment(token);
}

ASTNode* new_node_binary(NodeType type, ASTNode* l_value, ASTNode* r_value) {
	ASTNode* new_node = calloc(1, sizeof(ASTNode));
	new_node->l_value = l_value;
	new_node->r_value = r_value;
	new_node->node_type = type;
	return new_node;
}

ASTNode *parse_variable_assignment(Token **token) {
	ASTNode* left = parse_logical_or(token);

	if 	(	(*token)->token_type != TOKEN_EOF
		&&  (*token)->token_type == TOKEN_PUNCTUATOR
		&&  (*token)->punc_type  == PUNC_ASSIGNMENT   ) {
		*token = (*token)->next;

		// here instead we do recursive call because assignment links right to left 
		// i.e. a = b = c   <====> a = (b = c)
		ASTNode* right = parse_variable_assignment(token);
		return new_node_binary(NODE_ASSIGN, left, right);
	}

	// TODO: need to do +=, -= etc

	return left;
}

// ||
// just for intuition ill put lots of comments to explain how this works
ASTNode* parse_logical_or(Token** token) {
	ASTNode* left = parse_logical_and(token); // before we even consider parsing the logical or, we want to check if there is anything of higher presedence before it
	
	// it then looks at the current token, if punc type is not logical or, then we just skip this completely and return left, as there was no logical or operation to begin with

	// otherwise, we consume the logical or token, move past it, then it fetches whatevers next (ASTNode* right = parse_logical_and(token))
	while (  (*token)->token_type != TOKEN_EOF
		  && (*token)->token_type == TOKEN_PUNCTUATOR
		  && (*token)->punc_type  == PUNC_LOGICAL_OR  ) {
		*token = (*token)->next; 

		ASTNode* right = parse_logical_and(token);
		left = new_node_binary(NODE_LOGOR, left, right);
		// the new_node_binary part forms the tree, basically we have just done
		//		 LOG_OR
		//		/		\
		//	  left     right

		// then setting left to be this new formed tree, the new tree becomes the left side for the next iteration of the loop
	}

	return left;
}

// &&
ASTNode* parse_logical_and(Token** token) {
	ASTNode* left = parse_equality(token);

	while (  (*token)->token_type != TOKEN_EOF
		  && (*token)->token_type == TOKEN_PUNCTUATOR
		  && (*token)->punc_type  == PUNC_LOGICAL_AND  ) {
		*token = (*token)->next;

		ASTNode* right = parse_equality(token);
		left = new_node_binary(NODE_LOGAND, left, right);
	}

	return left;
}

// == or !=
ASTNode* parse_equality(Token** token) {
	ASTNode* left = parse_comparison(token);

	while (  (*token)->token_type != TOKEN_EOF
		  && (*token)->token_type == TOKEN_PUNCTUATOR
		  && ((*token)->punc_type == PUNC_INEQAULITY 
		  ||  (*token)->punc_type == PUNC_EQUALITY    )) {
		Punctuator type = (*token)->punc_type;
		*token = (*token)->next;

		ASTNode* right = parse_comparison(token);
		left = (type == PUNC_EQUALITY) ? new_node_binary(NODE_EQ, left, right) 
									   : new_node_binary(NODE_NE, left, right);
	}

	return left;
}

// <, >, <=, >=
ASTNode* parse_comparison(Token** token) {
	ASTNode* left = parse_term(token);

	while (  (*token)->token_type != TOKEN_EOF
		  && (*token)->token_type == TOKEN_PUNCTUATOR
		  && ((*token)->punc_type == PUNC_LESSTHAN 
		  ||  (*token)->punc_type == PUNC_GREATER    
		  ||  (*token)->punc_type == PUNC_GEQ
		  ||  (*token)->punc_type == PUNC_LEQ         )) {
		Punctuator type = (*token)->punc_type;
		*token = (*token)->next;

		ASTNode* right = parse_term(token);
		
		switch (type) {
			case PUNC_LESSTHAN: left = new_node_binary(NODE_LT, left, right); break;
			case PUNC_GREATER:  left = new_node_binary(NODE_GT, left, right); break;
			case PUNC_GEQ: 		left = new_node_binary(NODE_GE, left, right); break;
			case PUNC_LEQ: 		left = new_node_binary(NODE_LE, left, right); break;
		}
	}

	return left;
}

// +, -
ASTNode* parse_term(Token** token) {
	ASTNode* left = parse_factor(token);

	while (  (*token)->token_type != TOKEN_EOF
		  && (*token)->token_type == TOKEN_PUNCTUATOR
		  && ((*token)->punc_type == PUNC_ADDITION 
		  ||  (*token)->punc_type == PUNC_SUBTRACTION )) {
		Punctuator type = (*token)->punc_type;
		*token = (*token)->next;

		ASTNode* right = parse_factor(token);
		left = (type == PUNC_ADDITION) ? new_node_binary(NODE_ADD, left, right) 
									   : new_node_binary(NODE_SUB, left, right);
	}

	return left;
}

// *, /, %
ASTNode* parse_factor(Token** token) {
	ASTNode* left = parse_exponentiation(token);

	while (  (*token)->token_type != TOKEN_EOF
		  && (*token)->token_type == TOKEN_PUNCTUATOR
		  && ((*token)->punc_type == PUNC_MULTIPLY 
		  ||  (*token)->punc_type == PUNC_DIVIDE    
		  ||  (*token)->punc_type == PUNC_MOD         )) {
		Punctuator type = (*token)->punc_type;
		*token = (*token)->next;

		ASTNode* right = parse_exponentiation(token);
		
		switch (type) {
			case PUNC_MULTIPLY: left = new_node_binary(NODE_MUL, left, right); break;
			case PUNC_DIVIDE:  	left = new_node_binary(NODE_DIV, left, right); break;
			case PUNC_MOD: 		left = new_node_binary(NODE_MOD, left, right); break;
		}
	}

	return left;
}

// ** (this is right to left associative)
ASTNode* parse_exponentiation(Token** token) {
	ASTNode* left = parse_unary(token);

	if 	(	(*token)->token_type != TOKEN_EOF
		&&  (*token)->token_type == TOKEN_PUNCTUATOR
		&&  (*token)->punc_type  == PUNC_POW   		  ) {
		*token = (*token)->next;

		ASTNode* right = parse_exponentiation(token);
		return new_node_binary(NODE_EXP, left, right);
	}

	return left;
}

ASTNode* parse_unary(Token** token) {
	return parse_postfix(token);
}

ASTNode* parse_postfix(Token** token) {
	return parse_else(token);
}

ASTNode* parse_else(Token** token) {
	// belongs to Identifiers, Literals, Grouping
	return NULL;
}