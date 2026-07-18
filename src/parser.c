#include "../include/parser.h"
#include "../include/base.h"

#include <stdio.h>
#include <stdlib.h>

// the parser context is just a struct we use to encapsulate all the information the parser might need, without having to declare multiple global variables, which is bad practice

// you can check parser.h to see what exactly we use the parser context for but to put it simply, it just exists so that each function doesnt have to take loads of parameters:

// for example we only need ASTNode *parse_statement(ParserContext *ctx);
// 				instead of	ASTNode *parse_statement(Token **token, Scope *cur_scope, FunctionInfo *fun_info ... ) etc.

void initialise_parser_context(ParserContext *ctx, Token* head) {
	ctx->cur_function = NULL;
	ctx->cur_scope = NULL;
	ctx->cur_token = head;

#ifdef DEBUG
	ctx->variable_counter = 0;
#endif 

	initalise_global_scope(ctx);
}

void advance_token(ParserContext *ctx) {
	if (ctx->cur_token->token_type == TOKEN_EOF) return;
	ctx->cur_token = ctx->cur_token->next;
}

void initalise_global_scope(ParserContext *ctx) {
	ctx->cur_scope = calloc(1, sizeof(Scope));
	ctx->cur_scope->parent = NULL; // global scope is the top of the chain
	ctx->cur_scope->scope_depth = SCOPE_GLOBAL_DEPTH;
	ctx->cur_scope->symbols_head = NULL;
}

ASTNode *generate_ast(Token *head) {
	ParserContext ctx = {};
	initialise_parser_context(&ctx, head);

	ASTNode *root = NULL;

	while (ctx.cur_token->token_type != TOKEN_EOF) {
		// printf("%d\n", ctx.cur_token->token_type);
		ASTNode* cur_statement = parse_statement(&ctx);
		if (cur_statement == NULL) continue;

		if (root == NULL) root = cur_statement;
		else {
			ASTNode *end = root;
			for (; end->next != NULL; end = end->next);
			end->next = cur_statement;
		}
	}
	
	// printf("EOF\n");
	return root;
}

ASTNode *new_node_general(NodeType type, Token* tok) {
	ASTNode* new_node = calloc(1, sizeof(ASTNode));
	new_node->node_type = type;
	new_node->token = tok;
	return new_node;
}

// the scope system is just basically a heirarchy, structured like this for example

/*							 GLOBAL
 *						   /      \
 *                   main_scope   foo_scope
 * 						 |
 * 					block1_scope
 */

// this would be valid if we had a program like this

// fn main(void) -> void {						*				*
// 		foo();									|				|
// 		{					*					|	main_scope	|
// 			i32 x = 10;		|	block1_scope	|				|	GLOBAL
// 			x++;			|					|				|
// 		}					*					|				|
// }											*				|
//																|
// fn foo(void) -> void {						*				|
// 		print("bar", target=stdout);			|	foo_scope	|
// }											*				*

// symbols help us keep track of all the variables defined in the current scope
// the scope struct holds a linked list of symbols which were defined in that scope
// that scope is able to use all the symbols in its linked list, or any symbols in its parents' linked lists

// here is an example program:

// // program start
// i32 global_int_var = 0;
//
// fn main(void) -> void {
// 		i32 main_scope_var = 0;
// 		i32 main_scope_var_2 = 1;
// 		{	// this doesnt have a name but internally lets call it block1
// 			i32 nested_scope_var = 0;
// 		}
// }
// // program end

// then this would be the scope heirarchy

/*					  GLOBAL
 *						 |
 *                   main_scope   
 * 						 |
 * 					block1_scope
 */

// then GLOBAL.symbols_head 		= [global_int_var] // linked list
//		main_scope.symbols_head 	= [main_scope_var, main_scope_var2]
//		block1_scope.symbols_head 	= [nested_scope_var]

// so in main, i could use any of intersection([main_scope_var, main_scope_var2], [global_int_var])
// and in block1, i could use any of intersection([main_scope_var, main_scope_var2], [global_int_var], [nested_scope_var])
// but in main, i could NOT use nested_scope_var because i have no access to it

Symbol *new_symbol(ParserContext *ctx, char *sym_identifier, TypeInfo* typeinfo) {
	Symbol *new_symbol = calloc(1, sizeof(Symbol));
	new_symbol->name = sym_identifier;
	new_symbol->next = NULL;
	new_symbol->typeinfo = typeinfo;

#ifdef DEBUG
	new_symbol->variable_identifier = ctx->variable_counter;
	ctx->variable_counter++;
#endif

	// push to the end of the current scope's linked list of symbols
	if (ctx->cur_scope->symbols_head) {
		Symbol *end = ctx->cur_scope->symbols_head;
		for (; end->next != NULL; end = end->next);
		end->next = new_symbol;
	} else {
		ctx->cur_scope->symbols_head = new_symbol;
	}

	return new_symbol;
}

// helper function to match variable names to their corresponding symbols in the scope
Symbol *symbol_lookup(Scope *scope, char *sym_name) {
	assert(scope != NULL);

	size_t sym_len = strlen(sym_name);
	Scope* temp = scope;

	while (temp != NULL) {
		// go through this scopes symbols, if not there, then check the parents until we get back to GLOBAL scope
		Symbol* cur_symbol = temp->symbols_head;
		while (cur_symbol != NULL) {
			if (strlen(cur_symbol->name) == sym_len && strncmp(cur_symbol->name, sym_name, sym_len) == 0) return cur_symbol;
			cur_symbol = cur_symbol->next;
		}
		temp = temp->parent;
	}

	return NULL;
}

ASTNode *parse_variable_declaration(ParserContext *ctx) {
	printf("Parsing variable declaration\n");
	assert(ctx->cur_token->typeinfo != NULL);

    TypeInfo* typeinfo = ctx->cur_token->typeinfo;

    advance_token(ctx);
	if (ctx->cur_token->token_type != TOKEN_SYMBOL_IDENTIFIER) {
		ERR_SYNTAX(ctx->cur_token, /* expected a */ "variable identifer");
	}

	ASTNode *declaration_node = calloc(1, sizeof(ASTNode));
	declaration_node->node_type = NODE_VARAIBLE_DECLARATION;

    ASTNode *variable_node = calloc(1, sizeof(ASTNode));
    variable_node->node_type = NODE_VARIABLE;

    variable_node->variable_symbol = new_symbol(ctx, ctx->cur_token->lexeme, typeinfo);

	declaration_node->l_value = variable_node;

    advance_token(ctx);
    if (ctx->cur_token->token_type == TOKEN_PUNCTUATOR) {
		if (ctx->cur_token->punc_type == PUNC_ASSIGNMENT) {
			advance_token(ctx);

			ASTNode *value_node = parse_expression(ctx);
			
			if (ctx->cur_token->punc_type != PUNC_SEMICOLON) {
				ERR_SYNTAX(ctx->cur_token, /* expected a */ "';'");
			}

			declaration_node->r_value = value_node;

			advance_token(ctx);
			return declaration_node;
		} else if (ctx->cur_token->punc_type == PUNC_SEMICOLON) {
			advance_token(ctx);
			return declaration_node;
		}
    } else {
		ERR_SYNTAX(ctx->cur_token, /* expected a */ "';'");
	}
    
    return variable_node;
}

ASTNode *parse_function(ParserContext *ctx) {
	return NULL;
}

ASTNode *parse_statement(ParserContext *ctx) {
	printf("parsing statement\n");
	printf("%s\n", token_type_to_str(ctx->cur_token));
	DEBUG_TOKEN_STR(ctx->cur_token);
	switch (ctx->cur_token->token_type) {
		case TOKEN_KEYWORD_FUNCTION:
			// defining a function
			return parse_function(ctx);
		case TOKEN_PRIMITIVE_TYPE_SPECIFIER:
			printf("hit?\n");
			return parse_variable_declaration(ctx);
		case TOKEN_KEYWORD_IF:
			return parse_if_statement(ctx);
		case TOKEN_KEYWORD_WHILE:
			return parse_while_statement(ctx);
		case TOKEN_KEYWORD_FOR:
			return parse_for_statement(ctx);
		case TOKEN_KEYWORD_RETURN:
			return parse_return_statement(ctx);
		case TOKEN_PUNCTUATOR:
			if (ctx->cur_token->punc_type == PUNC_OPEN_CURLY) return parse_block(ctx);
		case TOKEN_SYMBOL_IDENTIFIER:
			// calling a function, or assigning a variable
			return parse_expr_statement(ctx);
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

ASTNode *parse_if_statement(ParserContext *ctx) {
	assert(ctx->cur_token->token_type == TOKEN_KEYWORD_IF);
	ASTNode* if_stmt = new_node_general(NODE_IF, ctx->cur_token);
	advance_token(ctx);

	if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
		|| 	ctx->cur_token->punc_type != PUNC_OPEN_PAREN	) {
		ERR_SYNTAX(ctx->cur_token, /* expected a */ "open parenthesis '(' for if clause");
	}
	advance_token(ctx); // consume (
	
	if_stmt->condition = parse_expression(ctx); // parse expression inside condition
	// ie if we hade if (age == 18) { ... } then if_stmt->condition would be age == 18 as an AST

	if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
		|| 	ctx->cur_token->punc_type != PUNC_CLOSE_PAREN	) {
		ERR_SYNTAX(ctx->cur_token, /* expected a */ "close parenthesis ')' for if clause");
	}
	advance_token(ctx); // consume )

	if_stmt->on_condition_success = parse_block(ctx);

	// DEBUG_TOKEN_STR(ctx->cur_token);
	// printf("%s\n", token_type_to_str(ctx->cur_token));
	if (ctx->cur_token->token_type == TOKEN_KEYWORD_ELSE) {
		advance_token(ctx);
		if_stmt->on_condition_failure = parse_block(ctx);
	}

	return if_stmt;
}

ASTNode *parse_while_statement(ParserContext *ctx) {
	TODO("parse while statement");
	return NULL;
}

ASTNode *parse_for_statement(ParserContext *ctx) {
	TODO("parse for statement");
	return NULL;
}

Scope *set_new_scope(ParserContext *ctx) {
	Scope *new_scope = calloc(1, sizeof(Scope));
	new_scope->parent = ctx->cur_scope;
	new_scope->scope_depth = ctx->cur_scope->scope_depth + 1;
	ctx->cur_scope = new_scope;

	return new_scope;
}

Scope *exit_scope(ParserContext *ctx) {
	ctx->cur_scope = ctx->cur_scope->parent;
	return ctx->cur_scope;
}

ASTNode *parse_block(ParserContext *ctx) {
	TODO("parse block");
	return NULL;
}

ASTNode *parse_return_statement(ParserContext *ctx) {
	TODO("parse return stmt");
	return NULL;
}

ASTNode *parse_expr_statement(ParserContext *ctx) {
	Token* ref = ctx->cur_token;
	ASTNode* expr = parse_expression(ctx);

	if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR
		|| 	ctx->cur_token->punc_type != PUNC_SEMICOLON		){
		// printf("%s %s", token_type_to_str(ctx->cur_token->token_type), punc_to_str(ctx->cur_token->punc_type));
		ERR_SYNTAX(ctx->cur_token, /* expected a */ "';' semicolon to end expression statement");
	}

	advance_token(ctx);
	ASTNode* statement = new_node_general(NODE_EXPR_STMT, ref);
	statement->l_value = expr;
	return statement;
}

// operator presedence (lowest to highest)

// name                                 symbols                 examples
// Assignment	                        =, +=, -=, *=, /=	    x = y + 2
// Logical OR	                        ||	                    a || b
// Logical AND	                        &&	                    a && b
// Equality	                            ==, !=	                status == 200
// Comparison	                        <, >, <=, >=	        age >= 18
// Bitwise Shift						<<, >>					x << 2
// Term (Additive)	                    +, -	                income - tax
// Factor (Multiplicative)	            *, /, %	                hours * wage
// Exponentiation						**						2 ** 5
// Unary / Prefix	                    !, ++, --	            !is_active, count--
// Postfix / Call	                    (), [], ., ->	        matrix[i][j]
// Identifiers, Literals, Grouping	                            x, 42, "hello", (a + b)

// i was looking up shunting yard, but apparently for an AST the easiest way to parse expressions is descent parsing

ASTNode *parse_expression(ParserContext *ctx) {
	return parse_variable_assignment(ctx);
}

ASTNode* new_node_binary(NodeType type, ASTNode* l_value, ASTNode* r_value, Token* tok) {
	ASTNode* new_node = calloc(1, sizeof(ASTNode));
	new_node->l_value = l_value;
	new_node->r_value = r_value;
	new_node->node_type = type;
	new_node->token = tok;
	return new_node;
}

ASTNode* new_node_unary(NodeType type, ASTNode* unary_val, Token* tok) {
	ASTNode* new_node = calloc(1, sizeof(ASTNode));
	new_node->r_value = unary_val;
	new_node->node_type = type;
	new_node->l_value = NULL;
	new_node->token = tok;
	return new_node;
}

ASTNode* new_node_memidentifier(ASTNode* l_value, char* identifier, Token* tok) {
	ASTNode* new_node = calloc(1, sizeof(ASTNode));
	new_node->l_value = l_value;
	new_node->node_type = NODE_MEMBER;
	new_node->r_value = NULL;
	new_node->token = tok;
	
	Symbol* mem_sym = calloc(1, sizeof(Symbol));
	mem_sym->name = identifier;
	new_node->variable_symbol = mem_sym;

	return new_node;
}

ASTNode *parse_variable_assignment(ParserContext *ctx) {
	ASTNode* left = parse_logical_or(ctx);

	if (	ctx->cur_token->token_type != TOKEN_EOF
		&& 	ctx->cur_token->token_type == TOKEN_PUNCTUATOR
		&& 	ctx->cur_token->punc_type  == PUNC_ASSIGNMENT	) {
		Token* ref = ctx->cur_token;
		advance_token(ctx);

		// here instead we do recursive call because assignment links right to left 
		// i.e. a = b = c   <====> a = (b = c)
		ASTNode* right = parse_variable_assignment(ctx);
		return new_node_binary(NODE_ASSIGN, left, right, ref);
	}

	if (	ctx->cur_token->token_type == TOKEN_PUNCTUATOR
		&& (ctx->cur_token->punc_type == PUNC_ADDEQ
		|| 	ctx->cur_token->punc_type == PUNC_SUBEQ
		|| 	ctx->cur_token->punc_type == PUNC_MULEQ
		|| 	ctx->cur_token->punc_type == PUNC_DIVEQ
		|| 	ctx->cur_token->punc_type == PUNC_MODEQ
		|| 	ctx->cur_token->punc_type == PUNC_ANDEQ
		|| 	ctx->cur_token->punc_type == PUNC_OREQ
		|| 	ctx->cur_token->punc_type == PUNC_XOREQ
		|| 	ctx->cur_token->punc_type == PUNC_RS_EQ
		|| 	ctx->cur_token->punc_type == PUNC_LS_EQ			)) {
		TODO("eq assignment operations");
	}

	return left;
}

// ||
// just for intuition ill put lots of comments to explain how this works
ASTNode* parse_logical_or(ParserContext* ctx) {
	ASTNode* left = parse_logical_and(ctx); // before we even consider parsing the logical or, we want to check if there is anything of higher presedence before it
	
	// it then looks at the current token, if punc type is not logical or, then we just skip this completely and return left, as there was no logical or operation to begin with

	// otherwise, we consume the logical or token, move past it, then it fetches whatevers next
	while (		ctx->cur_token->token_type != TOKEN_EOF
		  && 	ctx->cur_token->token_type == TOKEN_PUNCTUATOR
		  && 	ctx->cur_token->punc_type == PUNC_LOGICAL_OR	) {
		Token* ref = ctx->cur_token;
		advance_token(ctx);

		ASTNode* right = parse_logical_and(ctx);
		left = new_node_binary(NODE_LOGOR, left, right, ref);
		// the new_node_binary part forms the tree, basically we have just done
		//		 LOG_OR
		//		/		\
		//	  left     right

		// then setting left to be this new formed tree, the new tree becomes the left side for the next iteration of the loop
	}

	return left;
}

ASTNode* parse_logical_and(ParserContext* ctx) {
	ASTNode* left = parse_equality(ctx);

	while (		ctx->cur_token->token_type != TOKEN_EOF
		  && 	ctx->cur_token->token_type == TOKEN_PUNCTUATOR
		  && 	ctx->cur_token->punc_type == PUNC_LOGICAL_AND	) {
		Token* ref = ctx->cur_token;
		advance_token(ctx);

		ASTNode* right = parse_equality(ctx);
		left = new_node_binary(NODE_LOGAND, left, right, ref);
	}

	return left;
}

ASTNode* parse_equality(ParserContext* ctx) {
	ASTNode* left = parse_comparison(ctx);

	while (		ctx->cur_token->token_type != TOKEN_EOF
		  && 	ctx->cur_token->token_type == TOKEN_PUNCTUATOR
		  &&   (ctx->cur_token->punc_type == PUNC_INEQAULITY 
		  || 	ctx->cur_token->punc_type == PUNC_EQUALITY		)) {
		Token* ref = ctx->cur_token;
		Punctuator type = ctx->cur_token->punc_type;
		advance_token(ctx);

		ASTNode* right = parse_comparison(ctx);
		left = (type == PUNC_EQUALITY) ? new_node_binary(NODE_EQ, left, right, ref)
									   : new_node_binary(NODE_NE, left, right, ref);
	}

	return left;
}

ASTNode* parse_comparison(ParserContext* ctx) {
	ASTNode* left = parse_bitwise_shift(ctx);

	while (		ctx->cur_token->token_type != TOKEN_EOF
		  && 	ctx->cur_token->token_type == TOKEN_PUNCTUATOR
		  &&   (ctx->cur_token->punc_type == PUNC_LESSTHAN 
		  || 	ctx->cur_token->punc_type == PUNC_GREATER
		  || 	ctx->cur_token->punc_type == PUNC_GEQ 
		  || 	ctx->cur_token->punc_type == PUNC_LEQ			)) {
		Token* ref = ctx->cur_token;
		Punctuator type = ctx->cur_token->punc_type;
		advance_token(ctx);

		ASTNode* right = parse_bitwise_shift(ctx);
		
		switch (type) {
			case PUNC_LESSTHAN: left = new_node_binary(NODE_LT, left, right, ref); break;
			case PUNC_GREATER:  left = new_node_binary(NODE_GT, left, right, ref); break;
			case PUNC_GEQ: 		left = new_node_binary(NODE_GE, left, right, ref); break;
			case PUNC_LEQ: 		left = new_node_binary(NODE_LE, left, right, ref); break;
		}
	}

	return left;
}

ASTNode* parse_bitwise_shift(ParserContext* ctx) {
	ASTNode* left = parse_term(ctx);

	while (		ctx->cur_token->token_type != TOKEN_EOF
		  && 	ctx->cur_token->token_type == TOKEN_PUNCTUATOR
		  &&   (ctx->cur_token->punc_type == PUNC_LS
		  || 	ctx->cur_token->punc_type == PUNC_RS			)) {
		Token* ref = ctx->cur_token;
		Punctuator type = ctx->cur_token->punc_type;
		advance_token(ctx);

		ASTNode* right = parse_term(ctx);
		left = (type == PUNC_LS) 	? new_node_binary(NODE_SHL, left, right, ref)
									: new_node_binary(NODE_SHR, left, right, ref);
	}

	return left;
}

ASTNode* parse_term(ParserContext* ctx) {
	ASTNode* left = parse_factor(ctx);

	while (		ctx->cur_token->token_type != TOKEN_EOF
		  && 	ctx->cur_token->token_type == TOKEN_PUNCTUATOR
		  &&   (ctx->cur_token->punc_type == PUNC_ADDITION 
		  || 	ctx->cur_token->punc_type == PUNC_SUBTRACTION	)) {
		Token* ref = ctx->cur_token;
		Punctuator type = ctx->cur_token->punc_type;
		advance_token(ctx);

		ASTNode* right = parse_factor(ctx);
		left = (type == PUNC_ADDITION) ? new_node_binary(NODE_ADD, left, right, ref)
									   : new_node_binary(NODE_SUB, left, right, ref);
	}

	return left;
}

ASTNode* parse_factor(ParserContext* ctx) {
	ASTNode* left = parse_exponentiation(ctx);

	while (		ctx->cur_token->token_type != TOKEN_EOF
		  && 	ctx->cur_token->token_type == TOKEN_PUNCTUATOR
		  &&   (ctx->cur_token->punc_type == PUNC_MULTIPLY 
		  || 	ctx->cur_token->punc_type == PUNC_DIVIDE
		  || 	ctx->cur_token->punc_type == PUNC_MOD			)) {
		Token* ref = ctx->cur_token;
		Punctuator type = ctx->cur_token->punc_type;
		advance_token(ctx);

		ASTNode* right = parse_exponentiation(ctx);
		
		switch (type) {
			case PUNC_MULTIPLY: left = new_node_binary(NODE_MUL, left, right, ref); break;
			case PUNC_DIVIDE:   left = new_node_binary(NODE_DIV, left, right, ref); break;
			case PUNC_MOD: 		left = new_node_binary(NODE_MOD, left, right, ref); break;
		}
	}

	return left;
}

// (this is right to left associative)
ASTNode* parse_exponentiation(ParserContext* ctx) {
	ASTNode* left = parse_unary(ctx);

	if (	ctx->cur_token->token_type != TOKEN_EOF
		&& 	ctx->cur_token->token_type == TOKEN_PUNCTUATOR
		&& 	ctx->cur_token->punc_type == PUNC_POW			) {
		Token* ref = ctx->cur_token;
		advance_token(ctx);

		ASTNode* right = parse_exponentiation(ctx);
		return new_node_binary(NODE_EXP, left, right, ref);
	}

	return left;
}

ASTNode* parse_unary(ParserContext* ctx) {
	if (	ctx->cur_token->token_type != TOKEN_EOF 
		&& 	ctx->cur_token->token_type == TOKEN_PUNCTUATOR	) {
		Punctuator type = ctx->cur_token->punc_type;

		if (	type == PUNC_LOGICAL_NOT 
			|| 	type == PUNC_AMPERSAND 
			|| 	type == PUNC_BITWISE_NOT
			|| 	type == PUNC_SUBTRACTION 
			|| 	type == PUNC_MULTIPLY 		) {
			Token* ref = ctx->cur_token;
			advance_token(ctx);

			ASTNode* operand = parse_unary(ctx);
			
			switch (type) {
				case PUNC_LOGICAL_NOT: 	return new_node_unary(NODE_NOT, operand, ref);
				case PUNC_AMPERSAND: 	return new_node_unary(NODE_ADDR, operand, ref);
				case PUNC_BITWISE_NOT: 	return new_node_unary(NODE_BITNOT, operand, ref);
				case PUNC_SUBTRACTION: 	return new_node_unary(NODE_SUB, operand, ref);
				case PUNC_MULTIPLY: 	return new_node_unary(NODE_DEREF, operand, ref);
			}
		}
	}
	
	return parse_postfix(ctx);
}

ASTNode* parse_postfix(ParserContext* ctx) {
	ASTNode* left = parse_else(ctx);

	while (		ctx->cur_token->token_type != TOKEN_EOF 
		  && 	ctx->cur_token->token_type == TOKEN_PUNCTUATOR	) {
		Punctuator type = ctx->cur_token->punc_type;
		bool still_to_parse = true;
		Token* ref = ctx->cur_token;
		
		switch (type) {
			case PUNC_DOT:
			case PUNC_ARROW: {
				advance_token(ctx);

				if (ctx->cur_token->token_type != TOKEN_SYMBOL_IDENTIFIER) {
					ERR_SYNTAX(ctx->cur_token, /* expected a */ "member identifier");
				}

				left = new_node_memidentifier(left, ctx->cur_token->lexeme, ref);
				advance_token(ctx);
				break;
			}

			case PUNC_OPEN_SQUARE: {
				advance_token(ctx);
				ASTNode* right = parse_expression(ctx);

				if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
					|| 	ctx->cur_token->punc_type != PUNC_CLOSE_SQUARE	) {
					ERR_SYNTAX(ctx->cur_token, /* expected a */ "closing square bracket ']'" );
				}

				left = new_node_binary(NODE_INDEX, left, right, ref);
				advance_token(ctx);
				break;
			}

			case PUNC_OPEN_PAREN:
				left = parse_function_call(ctx, &left);
				break;

			case PUNC_INCREMENT:
			case PUNC_DECREMENT: {
				advance_token(ctx);
				left = (type == PUNC_INCREMENT) ? new_node_unary(NODE_INCREMENT, left, ref)
												: new_node_unary(NODE_DECREMENT, left, ref);
				break;
			}
			
			default:
				still_to_parse = false;
				break;
		}

		if (!still_to_parse) break;
	}

	return left;
}

ASTNode* parse_else(ParserContext* ctx) {
	switch (ctx->cur_token->token_type) {
		case TOKEN_PUNCTUATOR: {
			if (ctx->cur_token->punc_type != PUNC_OPEN_PAREN) break;
			advance_token(ctx);
			ASTNode* subexpr = parse_expression(ctx);

			if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
				|| 	ctx->cur_token->punc_type != PUNC_CLOSE_PAREN	) {
				ERR_SYNTAX(ctx->cur_token, /* expected a */ "closed expression, closed by ')'" );
			}
			
			advance_token(ctx);
			return subexpr;
		}

		case TOKEN_STRING_LITERAL: {
            ASTNode* str_node = calloc(1, sizeof(ASTNode));
            str_node->node_type = NODE_LITERAL_STRING;
            str_node->token = ctx->cur_token;
            
            advance_token(ctx);
            return str_node;
        }
		
		case TOKEN_SYMBOL_IDENTIFIER: {
            ASTNode* var_node = calloc(1, sizeof(ASTNode));
			NodeType type = NODE_VARIABLE;

			Token* peek = ctx->cur_token->next;
			if (	peek != NULL 
				&& 	peek->token_type == TOKEN_PUNCTUATOR 
				&& 	peek->punc_type == PUNC_OPEN_PAREN		) {
				type = NODE_FUNCTION;
			}

            var_node->node_type = type;

			Symbol* sym = symbol_lookup(ctx->cur_scope, ctx->cur_token->lexeme);
			if (sym == NULL) ERR_SYNTAX(ctx->cur_token, /* expected a */ "previously declared identifier in the scope");
            var_node->variable_symbol = sym;
            var_node->token = ctx->cur_token;
            
            advance_token(ctx);
            return var_node;
        }

		case TOKEN_INT_LITERAL:
		case TOKEN_FLOAT_LITERAL: {
			ASTNode* val = calloc(1, sizeof(ASTNode));
			val->node_type = ctx->cur_token->token_type == TOKEN_INT_LITERAL ? NODE_LITERAL_INT : NODE_LITERAL_FLOAT;

			val->token = ctx->cur_token;
			advance_token(ctx);
			return val;
		}

		case TOKEN_NULL: {
			ASTNode* null_val = calloc(1, sizeof(ASTNode));
			null_val->node_type = NODE_NULL_EXPR;
			advance_token(ctx);
			return null_val;
		}

		default: break;
	}
	
	ERR_SYNTAX(ctx->cur_token, /* expected a */ "valid expression");
	return NULL;
}

ASTNode* parse_function_call(ParserContext* ctx, ASTNode** rest) {
	assert(	ctx->cur_token->token_type == TOKEN_PUNCTUATOR 
		&& 	ctx->cur_token->punc_type == PUNC_OPEN_PAREN		);

	Token* ref = ctx->cur_token;
	advance_token(ctx);
	ASTNode* func_call = calloc(1, sizeof(ASTNode));
	func_call->l_value = *rest;
	func_call->node_type = NODE_FUNCTION_CALL;
	func_call->token = ref;

	ASTNode* args = NULL;
	if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
		|| 	ctx->cur_token->punc_type != PUNC_CLOSE_PAREN	) {
		for (bool more_args = true; more_args;) {
			ASTNode* cur_arg = parse_expression(ctx);
		
			if (args == NULL) args = cur_arg;
			else {
				ASTNode* temp = args;
				for (; temp->next != NULL; temp = temp->next);
				temp->next = cur_arg;
			}

			more_args = 	ctx->cur_token->token_type == TOKEN_PUNCTUATOR 
						&& 	ctx->cur_token->punc_type == PUNC_COMMA;
			if (more_args) advance_token(ctx);
		}
	}

	if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
		|| 	ctx->cur_token->punc_type != PUNC_CLOSE_PAREN	) {
		ERR_SYNTAX(ctx->cur_token, /* expected a */ "')' to close function arguments");
	}

	advance_token(ctx);
	func_call->body = args;
	return func_call;
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
		case NODE_SHL:					return "SHL (<<)";
		case NODE_SHR:					return "SHR (>>)";
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
		case NODE_FUNCTION:				return "FUNCTION";
        case NODE_FUNCTION_CALL:        return "FUNCTION_CALL ()";
		case NODE_NULL_EXPR:			return "NULL";
		case NODE_EXPR_STMT:			return "EXPRESSION STMT";
        default:                        return "UNKNOWN_NODE";
    }
}

void trace(ASTNode* head, size_t depth) {
	for (size_t i = 0; i < depth; i++) printf("  ");
	printf("%s", node_to_str(head->node_type));

	if (head->node_type == NODE_VARIABLE || head->node_type == NODE_MEMBER || head->node_type == NODE_FUNCTION)
		printf(" [\"%s\"]", head->variable_symbol->name);

	if (head->node_type == NODE_VARIABLE) {
		TypeInfo* t_info = head->variable_symbol->typeinfo;
		printf(" [type=%s]", type_to_str(t_info->type, t_info->is_unsigned));
		printf(" [pdepth=%hu]", t_info->pointer_depth);
		if (t_info->pointer_depth > 0) printf(" [%s]", t_info->is_optional ? "nullable" : "nonnull");
		printf(" [vari=%lu]", head->variable_symbol->variable_identifier);
	}

	if (head->node_type == NODE_LITERAL_INT) printf(" [%lu]", head->token->int_val);
	if (head->node_type == NODE_LITERAL_FLOAT) printf(" [%Lf]", head->token->float_val);
	if (head->node_type == NODE_LITERAL_STRING) printf(" [\"%s\"]", head->token->str_val);
	printf("\n");

	if (head->l_value != NULL) trace(head->l_value, depth + 1);
	if (head->r_value != NULL) trace(head->r_value, depth + 1);
	if (head->body != NULL) trace(head->body, depth + 1);
	if (head->next != NULL) trace(head->next, depth);
}

#endif
