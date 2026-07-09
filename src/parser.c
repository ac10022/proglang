#include "../include/parser.h"
#include "../include/base.h"

#include <stdio.h>
#include <stdlib.h>

ASTNode *generate_ast(Token *head) {
	Token *current_token = head;
	Token **current = &current_token;

	ASTNode *root;

	// TODO: they all need to stem from a program start node or something
	while ( (*current)->token_type != TOKEN_EOF ) {
		printf("%d\n", (*current)->token_type);
		root = parse_statement(current);
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

ASTNode* new_node_unary(NodeType type, ASTNode* unary_val) {
	ASTNode* new_node = calloc(1, sizeof(ASTNode));
	new_node->r_value = unary_val;
	new_node->node_type = type;
	new_node->l_value = NULL;
	return new_node;
}

ASTNode* new_node_memidentifier(ASTNode* l_value, char* identifier) {
	ASTNode* new_node = calloc(1, sizeof(ASTNode));
	new_node->l_value = l_value;
	new_node->node_type = NODE_MEMBER;
	new_node->r_value = NULL;
	new_node->symbol_identifier = identifier;
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

// !, ++, --
ASTNode* parse_unary(Token** token) {
	if (	(*token)->token_type != TOKEN_EOF
		&&  (*token)->token_type == TOKEN_PUNCTUATOR  ) {
		Punctuator type = (*token)->punc_type;

		if (	type == PUNC_LOGICAL_NOT
			|| 	type == PUNC_AMPERSAND
			||  type == PUNC_BITWISE_NOT 
			|| 	type == PUNC_SUBTRACTION
			|| 	type == PUNC_MULTIPLY
			||  type == PUNC_INCREMENT
			|| 	type == PUNC_DECREMENT		) {
			*token = (*token)->next;

			ASTNode* operand = parse_unary(token); // in case we get like !!true to
			
			switch (type) {
				case PUNC_LOGICAL_NOT: 	return new_node_unary(NODE_NOT, operand);
				case PUNC_AMPERSAND: 	return new_node_unary(NODE_ADDR, operand);
				case PUNC_BITWISE_NOT: 	return new_node_unary(NODE_BITNOT, operand);
				case PUNC_SUBTRACTION: 	return new_node_unary(NODE_SUB, operand);
				case PUNC_MULTIPLY: 	return new_node_unary(NODE_DEREF, operand);
				case PUNC_INCREMENT: 	return new_node_unary(NODE_INCREMENT, operand);
				case PUNC_DECREMENT: 	return new_node_unary(NODE_DECREMENT, operand);
			}
		}
	}
	
	return parse_postfix(token);
}

// (), [], ., ->
ASTNode* parse_postfix(Token** token) {
	ASTNode* left = parse_else(token);

	while (  (*token)->token_type != TOKEN_EOF
		  && (*token)->token_type == TOKEN_PUNCTUATOR	) {
		Punctuator type = (*token)->punc_type;
		bool still_to_parse = true;
		
		switch (type) {
			case PUNC_DOT:
			case PUNC_ARROW: {
				*token = (*token)->next; // consume arrow or dot

				if ((*token)->token_type != TOKEN_SYMBOL_IDENTIFIER) {
					ERR_SYNTAX(*token, /* expected a */ "member identifier");
				}

				left = new_node_memidentifier(left, (*token)->lexeme);
				*token = (*token)->next; // then consume member identifier
				break;
			}

			case PUNC_OPEN_SQUARE: {
				*token = (*token)->next; // consume open square
				ASTNode* right = parse_expression(token);

				if (	(*token)->token_type != TOKEN_PUNCTUATOR
					|| 	(*token)->punc_type != PUNC_CLOSE_SQUARE	) {
					ERR_SYNTAX(*token, /* expected a */ "closing square bracket ']'");
				}

				left = new_node_binary(NODE_INDEX, left, right);
				*token = (*token)->next; // consume close square
				break;
			}

			case PUNC_OPEN_PAREN:
				// TODO: parse function call this is gonan be a pain in the ass
				parse_function_call(token);
				/* this is DEBUG remove it */ while ((*token)->token_type != TOKEN_PUNCTUATOR || (*token)->punc_type != PUNC_CLOSE_PAREN) *token = (*token)->next;
				*token = (*token)->next; // skip close paren
				break;
			
			default: 
				still_to_parse = false; 
				break;
		}

		if (!still_to_parse) break;
	}

	return left;
}

// Identifiers, Literals, Grouping
ASTNode* parse_else(Token** token) {
	switch ((*token)->token_type) {
		case TOKEN_PUNCTUATOR: {
			if ((*token)->punc_type != PUNC_OPEN_PAREN) break; // i might be forgetting something but i think this is the only remaining punctuator case
			*token = (*token)->next;
			ASTNode* subexpr = parse_expression(token);

			if (	(*token)->token_type != TOKEN_PUNCTUATOR
				||	(*token)->punc_type != PUNC_CLOSE_PAREN		) {
				ERR_SYNTAX(*token, /* expected a */ "closed expression, closed by ')'");
			}
			
			*token = (*token)->next; // consume close paren
			return subexpr;
		}

		case TOKEN_STRING_LITERAL: {
            ASTNode* str_node = calloc(1, sizeof(ASTNode));
            str_node->node_type = NODE_LITERAL_STRING;
            str_node->token = *token;
            
            *token = (*token)->next;
            return str_node;
        }
		
		case TOKEN_SYMBOL_IDENTIFIER: { 	/* this is gonna name of a variable, like 'x'*/
            ASTNode* var_node = calloc(1, sizeof(ASTNode));
            var_node->node_type = NODE_VARIABLE; // 
            var_node->symbol_identifier = (*token)->lexeme;
            var_node->token = *token;
            
            *token = (*token)->next;
            return var_node;
        }

		case TOKEN_INT_LITERAL:
		case TOKEN_FLOAT_LITERAL: {
			ASTNode* val = calloc(1, sizeof(ASTNode));
			val->node_type = (*token)->token_type == TOKEN_INT_LITERAL ? NODE_LITERAL_INT : NODE_LITERAL_FLOAT;

			val->token = *token; // if you want to get the actual literal value you can just do val->token->float_val or something
			*token = (*token)->next;
			return val;
		}

		default: break;
	}
	
	ERR_SYNTAX(*token, /* expected a */ "valid expression"); // i dont know what better error message to put here, if you get here somehow you really fuck up
	return NULL;
}

// heirarchy ends here

// this is not function definition, rather its just for calling them
// i.e. i32 res = add(1, 2);
//				     ^^^^^^ <- this function will be triggered here
ASTNode* parse_function_call(Token** token) {
	// helper for the postfix part
	return NULL;
}

#ifdef DEBUG 

const char* node_to_str(NodeType type) {
    switch (type) {
        case NODE_VARAIBLE_DECLARATION: return "VARIABLE_DECLARATION";
        case NODE_VARIABLE:             return "VARIABLE";
        case NODE_LITERAL_INT:          return "LITERAL_INT";
        case NODE_LITERAL_FLOAT:        return "LITERAL_FLOAT";
        case NODE_LITERAL_STRING:       return "LITERAL_STRING";
        case NODE_ASSIGN:               return "ASSIGN (=)";
        case NODE_ADD:                  return "ADD (+)";
        case NODE_SUB:                  return "SUB (-)";
        case NODE_MUL:                  return "MUL (*)";
        case NODE_DIV:                  return "DIV (/)";
        case NODE_MOD:                  return "MOD (%)";
        case NODE_EXP:                  return "EXP (**)";
        case NODE_LOGOR:                return "LOGICAL_OR (||)";
        case NODE_LOGAND:               return "LOGICAL_AND (&&)";
        case NODE_EQ:                   return "EQUAL (==)";
        case NODE_NE:                   return "NOT_EQUAL (!=)";
        case NODE_LT:                   return "LESS_THAN (<)";
        case NODE_GT:                   return "GREATER_EQUAL (>)";
        case NODE_LE:                   return "LESS_EQUAL (<=)";
        case NODE_GE:                   return "GREATER_EQUAL (>=)";
        case NODE_NOT:                  return "UNARY_NOT (!)";
        case NODE_ADDR:                 return "ADDRESS_OF (&)";
        case NODE_BITNOT:               return "BITWISE_NOT (~)";
        case NODE_INCREMENT:            return "INCREMENT (++)";
        case NODE_DECREMENT:            return "DECREMENT (--)";
        case NODE_DEREF:                return "DEREFERENCE (*)";
        case NODE_INDEX:                return "ARRAY_INDEX []";
        case NODE_MEMBER:               return "STRUCT_MEMBER (. or ->)";
        case NODE_FUNCTION_CALL:        return "FUNCTION_CALL ()";
        default:                        return "UNKNOWN_NODE";
    }
}

void trace(ASTNode* head, size_t depth) {
	for (size_t i = 0; i < depth; i++) printf("  ");
	printf("%s", node_to_str(head->node_type));

	if ((head->node_type == NODE_VARIABLE || head->node_type == NODE_MEMBER)) printf(" [\"%s\"]", head->symbol_identifier);
	if (head->node_type == NODE_LITERAL_INT) printf(" [%lu]", head->token->int_val);
	if (head->node_type == NODE_LITERAL_FLOAT) printf(" [%Lf]", head->token->float_val);
	if (head->node_type == NODE_LITERAL_STRING) printf(" [\"%s\"]", head->token->str_val);
	printf("\n");

	if (head->l_value != NULL) trace(head->l_value, depth + 1);
	if (head->r_value != NULL) trace(head->r_value, depth + 1);
}

#endif