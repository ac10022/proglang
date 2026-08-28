#include "parser.h"

#include <stdio.h>
#include <stdlib.h>

/*
 * TODO
	* break/continue/import statements
	* array types
	* named arguments, e.g., print("hello world", target=stdout)
 */

// the parser context is just a struct we use to encapsulate all the information the parser might need, without having to declare multiple global variables, which is bad practice

// you can check parser.h to see what exactly we use the parser context for but to put it simply, it just exists so that each function doesnt have to take loads of parameters:

// for example we only need ASTNode *parse_statement(ParserContext *ctx);
// 				instead of	ASTNode *parse_statement(Token **token, Scope *cur_scope, FunctionInfo *fun_info ... ) etc.

void initialise_parser_context(ParserContext *ctx, Token* head, CompilerContext *c_ctx) {
	ctx->cur_function = NULL;
	ctx->cur_scope = NULL;
	ctx->cur_token = head;
	ctx->cl_ctx = c_ctx->cl_ctx;
	ctx->arena = c_ctx->arena;

#ifdef DEBUG
	ctx->variable_counter = (size_t)0;
#endif 

	initialise_global_scope(ctx);
}

void initialise_global_scope(ParserContext *ctx) {
	ctx->cur_scope = PALLOCT(ctx->arena, Scope, 1);
	ctx->cur_scope->parent = NULL; // global scope is the top of the chain
	ctx->cur_scope->scope_depth = SCOPE_GLOBAL_DEPTH;
	ctx->cur_scope->symbols_head = NULL;
}

/*
 * Helper function to advance the token the context is looking at.
 */
void advance_token(ParserContext *ctx) {
	if (ctx->cur_token->token_type == TOKEN_EOF) return;
	ctx->cur_token = ctx->cur_token->next;
}

/*
 * Main entry point of the PARSER component; accepts a Token linked list from the lexer and returns the corresponding AST, whilst checking for syntax.
 */
ASTNode *generate_ast(Token *head, CompilerContext *c_ctx) {
	ParserContext ctx = {};
	initialise_parser_context(&ctx, head, c_ctx);

	ASTNode *root = NULL;

	while (ctx.cur_token->token_type != TOKEN_EOF) {
		ASTNode* cur_statement = parse_statement(&ctx);
		if (cur_statement == NULL) continue;

		if (root == NULL) root = cur_statement;
		else {
			ASTNode *end = root;
			for (; end->next != NULL; end = end->next);
			end->next = cur_statement;
		}
	}
	
	return root;
}

ASTNode *new_node_general(ParserContext* p_ctx, NodeType type, Token* tok) {
	ASTNode* new_node = PALLOCT(p_ctx->arena, ASTNode, 1);
	new_node->node_type = type;
	new_node->token = tok;
	return new_node;
}

ASTNode* new_node_binary(ParserContext* p_ctx, NodeType type, ASTNode* l_value, ASTNode* r_value, Token* tok) {
	ASTNode* new_node = PALLOCT(p_ctx->arena, ASTNode, 1);
	new_node->l_value = l_value;
	new_node->r_value = r_value;
	new_node->node_type = type;
	new_node->token = tok;
	return new_node;
}

ASTNode* new_node_unary(ParserContext* p_ctx, NodeType type, ASTNode* unary_val, Token* tok) {
	ASTNode* new_node = PALLOCT(p_ctx->arena, ASTNode, 1);
	new_node->r_value = unary_val;
	new_node->node_type = type;
	new_node->l_value = NULL;
	new_node->token = tok;
	return new_node;
}

ASTNode* new_node_memidentifier(ParserContext* p_ctx, ASTNode* l_value, char* identifier, Token* tok) {
	ASTNode* new_node = PALLOCT(p_ctx->arena, ASTNode, 1);
	new_node->l_value = l_value;
	new_node->node_type = NODE_MEMBER;
	new_node->r_value = NULL;
	new_node->token = tok;
	
	Symbol* mem_sym = PALLOCT(p_ctx->arena, Symbol, 1);
	mem_sym->name = identifier;
	new_node->variable_symbol = mem_sym;

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

// so in main, i could use any of union([main_scope_var, main_scope_var2], [global_int_var])
// and in block1, i could use any of union([main_scope_var, main_scope_var2], [global_int_var], [nested_scope_var])
// but in main, i could NOT use nested_scope_var because i have no access to it

/*
 * Create a new child scope off the current scope and set the context's scope to this new one.
 */
Scope *set_new_scope(ParserContext *ctx) {
	Scope *new_scope = PALLOCT(ctx->arena, Scope, 1);
	new_scope->parent = ctx->cur_scope;
	new_scope->scope_depth = ctx->cur_scope->scope_depth + 1;
	ctx->cur_scope = new_scope;

	return new_scope;
}

/*
 * Exit the current scope and move to parent scope.
 */
Scope *exit_scope(ParserContext *ctx) {
	// how would you get here? idk but better be safe
	if (ctx->cur_scope->scope_depth == SCOPE_GLOBAL_DEPTH) {
		ERR_GENERAL("cannot exit global scope");
	}

	ctx->cur_scope = ctx->cur_scope->parent;
	return ctx->cur_scope;
}

/*
 * Create a new symbol and push it onto the current context's scope.
 */
Symbol *new_symbol(ParserContext *ctx, char *sym_identifier, TypeInfo* typeinfo) {
	// check symbol does not already exist
	if (symbol_lookup(ctx->cur_scope, sym_identifier) != NULL) {
		ERR_SEMANTIC_CTX(ctx->cl_ctx, ctx->cur_token, "trying to declare a variable with an identifier already held by another variable in the same scope", true);
	}
	
	Symbol *new_symbol = PALLOCT(ctx->arena, Symbol, 1);
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

/*
 * Helper function to match variable names to their corresponding symbols in the scope, returns the symbol the variable name is referencing in this scope if found, otherwise returns NULL.
 */
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

/*
 * Helper function to retrieve a pointer to the global scope. 
 */
Scope* get_global_scope(ParserContext *ctx) {
	Scope* ref = ctx->cur_scope;
	while (ref->parent != NULL) ref = ref->parent;
	assert(ref->scope_depth == SCOPE_GLOBAL_DEPTH);
	return ref;
}

/*
 * Helper function to lookup whether a function has already been defined.
 * This will return the NODE_FUNCTION ASTNode of the function if we have already defined this function, otherwise will return NULL if it is not found (i.e. does not exist).
 */
ASTNode *function_lookup(ParserContext *ctx, char *function_name) {
	Scope* global = get_global_scope(ctx);
	assert(global != NULL);

	size_t fun_len = strlen(function_name);

	Function* cur_fun = global->functions_head;
	while (cur_fun != NULL) {
		if (strlen(cur_fun->func_node->function_name) == fun_len && strncmp(cur_fun->func_node->function_name, function_name, fun_len) == 0) return cur_fun->func_node;
		cur_fun = cur_fun->next;
	}

	return NULL;
}

/*
 * A helper function to get the parameter count of a function's signature.
 * e.g.,
 * 	fn foo(T1 a, T2 b, T3 c) -> T { ... }
 * will have a parameter count of 3. 
 */
size_t function_get_param_count(ASTNode *function_node) {
	assert(function_node->node_type == NODE_FUNCTION);

	if (function_node->l_value == NULL) return (size_t)0;
	size_t param_count = 1;

	ASTNode *end = function_node->l_value;
	for (; end->next != NULL; end = end->next) param_count++;
	return param_count;
}

/*
 * Appends the current function (context's cur_function) into the global function linked list.
 */
void add_cur_function_to_global_scope(ParserContext *ctx) {
	assert(ctx->cur_function != NULL);

	// function with this name already exists
	if (function_lookup(ctx, ctx->cur_function->function_name) != NULL) {
		ERR_SEMANTIC_CTX(ctx->cl_ctx, ctx->cur_token, "trying to declare a function with an identifier already taken", true);
	}

	Scope* global = get_global_scope(ctx);
	assert(global != NULL);

	Function* fun = PALLOCT(ctx->arena, Function, 1);
	fun->func_node = ctx->cur_function;
	fun->next = NULL;

	// push function to end of global function linked list
	if (global->functions_head) {
		Function *end = global->functions_head;
		for (; end->next != NULL; end = end->next);
		end->next = fun;
	} else {
		global->functions_head = fun;
	}
}

ASTNode *parse_statement(ParserContext *ctx) {
	switch (ctx->cur_token->token_type) {
		case TOKEN_KEYWORD_FUNCTION:			return parse_function(ctx);
		case TOKEN_PRIMITIVE_TYPE_SPECIFIER:	return parse_variable_declaration(ctx, true, NULL);
		case TOKEN_KEYWORD_IF:					return parse_if_statement(ctx);
		case TOKEN_KEYWORD_WHILE:				return parse_while_statement(ctx);
		case TOKEN_KEYWORD_FOR:					return parse_for_statement(ctx);
		case TOKEN_KEYWORD_RETURN:				return parse_return_statement(ctx);
		case TOKEN_SYMBOL_IDENTIFIER:			return parse_expr_statement(ctx);

		case TOKEN_PUNCTUATOR:
			if (ctx->cur_token->punc_type == PUNC_OPEN_CURLY) return parse_block(ctx);

			if (ctx->cur_token->punc_type == PUNC_MULTIPLY) TODO("handle case of e.g., *p = 10");
			if (ctx->cur_token->punc_type == PUNC_OPEN_PAREN) TODO("handle case of e.g., (tok)->next = NULL"); 

			else ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "valid statement token", true);
			return NULL; // to shut up compiler
		
		default:
			ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "valid statement token", true);
			return NULL; // to shut up compiler
	}
}

ASTNode *parse_variable_declaration(ParserContext *ctx, bool expect_semicolon, bool* declared) {
#ifdef DEBUG
	printf("Parsing variable declaration\n");
#endif
	assert(ctx->cur_token->typeinfo != NULL);

    TypeInfo* typeinfo = ctx->cur_token->typeinfo;

    advance_token(ctx);
	if (ctx->cur_token->token_type != TOKEN_SYMBOL_IDENTIFIER) {
		ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "variable identifer", true);
	}

	ASTNode *declaration_node = PALLOCT(ctx->arena, ASTNode, 1);
	declaration_node->node_type = NODE_VARAIBLE_DECLARATION;

    ASTNode *variable_node = PALLOCT(ctx->arena, ASTNode, 1);
    variable_node->node_type = NODE_VARIABLE;

    variable_node->variable_symbol = new_symbol(ctx, ctx->cur_token->lexeme, typeinfo);

	declaration_node->l_value = variable_node;

    advance_token(ctx);
    if (ctx->cur_token->token_type == TOKEN_PUNCTUATOR) {
		if (ctx->cur_token->punc_type == PUNC_ASSIGNMENT) {
			advance_token(ctx);

			ASTNode *value_node = parse_expression(ctx);
			
			if (expect_semicolon && ctx->cur_token->punc_type != PUNC_SEMICOLON) {
				ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "';'", true);
			}

			declaration_node->r_value = value_node;

			if (expect_semicolon) advance_token(ctx);
			if (declared) *declared = true;
			return declaration_node;
		} else if (expect_semicolon && ctx->cur_token->punc_type == PUNC_SEMICOLON) {
			if (expect_semicolon) advance_token(ctx);
			return declaration_node;
		}
    } else if (expect_semicolon) {
		ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "';'", true);
	}
    
    return declaration_node;
}

/*
 * Parses a single function parameter from the function template.
 * Returns a NODE_PARAMETER node, which holds the parameter symbol in the variable_symbol field.
 * TODO: currently only supports primitive types, functionality needs to be extended to support struct types.
 */
ASTNode *parse_function_parameter(ParserContext *ctx) {
	if (ctx->cur_token->token_type != TOKEN_PRIMITIVE_TYPE_SPECIFIER) {
		// this MAY be triggered later because we have no parsing for struct or non-primitive types yet, for instance if we had idk a user-defined Time struct later, this would call on
		// fn foo(Time t) -> void { ... }
		// because Time is not primitive
		ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "valid type specifier for parameter", true);
	}

	TypeInfo* param_type = ctx->cur_token->typeinfo;
	Token* type_ref = ctx->cur_token;
	advance_token(ctx); // consume type specifier

	if (ctx->cur_token->token_type != TOKEN_SYMBOL_IDENTIFIER) {
		ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "valid identifier for function parameter", true);
	}

	char* param_name = ctx->cur_token->lexeme;
	Token* name_ref = ctx->cur_token;

	Symbol* param = new_symbol(ctx, param_name, param_type);
	advance_token(ctx);

	ASTNode* param_node = new_node_general(ctx, NODE_PARAMETER, type_ref);
	param_node->variable_symbol = param;

	return param_node;
}

/*
 * Parses an function statement.
 * A function statement has forms:
 * 
 * 	fn foo(T1 arg1, T2 arg2, ..., TN argn) -> T { ... }		
 * 		// foo :: T1 -> T2 -> ... -> TN -> T
 * 
 *  fn foo(void) -> T { ... }
 * 		// foo :: void -> T
 * 
 *  fn foo() { ... }		
 * 		// foo :: void -> void
 * 		
 * Returns a NODE_FUNCTION node.
 * The function name is stored in the function_name field.
 * The parameters are stored as a linked list in the l_value field.
 * The return type is stored in the function_return_type field.
 * The NODE_BLOCK which makes up the function body is stored in the body field.
 */
ASTNode *parse_function(ParserContext *ctx) {
	assert(ctx->cur_token->token_type == TOKEN_KEYWORD_FUNCTION);
	ASTNode* func = new_node_general(ctx, NODE_FUNCTION, ctx->cur_token);
	advance_token(ctx);

	// function identifier, can be accessed through node->token->lexeme
	if (ctx->cur_token->token_type != TOKEN_SYMBOL_IDENTIFIER) {
		ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "valid identifier for function", true);
	}
	func->function_name = ctx->cur_token->lexeme;
	advance_token(ctx);

	if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
		|| 	ctx->cur_token->punc_type != PUNC_OPEN_PAREN	) {
		ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "open parenthesis '(' for potential function arguments", true);
	}
	advance_token(ctx); // consume (

	// parse parameters in a new scope (so param placeholders cannot be used outside of the function)
	set_new_scope(ctx);
	ASTNode* params = NULL;

	//									  vvvv
	// case function defined like fn main(void) -> ... { ... }
	if (	ctx->cur_token->token_type == TOKEN_PRIMITIVE_TYPE_SPECIFIER
		&& 	ctx->cur_token->typeinfo->type == TYPE_VOID					) {
		advance_token(ctx); // skip 'void'
	}

	// general function definition fn foo(T1 a, T2 b ... ) -> T3 { ... } 
	else if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
			|| 	ctx->cur_token->punc_type != PUNC_CLOSE_PAREN	) {
		for (bool more_params = true; more_params;) {
			ASTNode* cur_param = parse_function_parameter(ctx);
		
			if (params == NULL) params = cur_param;
			else {
				ASTNode* temp = params;
				for (; temp->next != NULL; temp = temp->next);
				temp->next = cur_param;
			}

			more_params = 	ctx->cur_token->token_type == TOKEN_PUNCTUATOR 
						&& 	ctx->cur_token->punc_type == PUNC_COMMA;
			if (more_params) advance_token(ctx);
		}
	}

	func->l_value = params;

	if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
		|| 	ctx->cur_token->punc_type != PUNC_CLOSE_PAREN	) {
		ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "closing parenthesis ')' after function parameters", true);
	}
	advance_token(ctx); // consume )

	// function has an arrow + return type specifier 
	if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
		|| 	ctx->cur_token->punc_type != PUNC_OPEN_CURLY	) {
			if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
				|| 	ctx->cur_token->punc_type != PUNC_ARROW	) {
				ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "arrow before declaring function return type", true);
			}
			advance_token(ctx); // consume ->
		
			if (ctx->cur_token->token_type != TOKEN_PRIMITIVE_TYPE_SPECIFIER) {
				// this MAY be triggered later because we have no parsing for struct or non-primitive types yet, for instance if we had idk a user-defined Time struct later, this would call on
				// fn foo(void) -> Time { ... }
				// because Time is not primitive
				ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "valid type specifier for function return type", true);
			}
		
			TypeInfo* return_type = ctx->cur_token->typeinfo;
			func->function_return_type = return_type;
			advance_token(ctx); // consume return type
	}

	// function does not have a specified return type
	// e.g., fn main() { ... }
	// this is legal
	// we should just infer the return type is VOID
	else {
		func->function_return_type = PALLOCT(ctx->arena, TypeInfo, 1);
		SET_TYPE_VOID(func->function_return_type);
	}

	ASTNode* enclosing_func = ctx->cur_function;
	ctx->cur_function = func;
	add_cur_function_to_global_scope(ctx);

	func->body = parse_block(ctx);

	exit_scope(ctx);
	ctx->cur_function = enclosing_func;

	return func;
}

/*
 * Parses an if statement.
 * Returns a NODE_IF node, the condition is stored in the ASTNode's condition field.
 * If the condition succeeds, the NODE_BLOCK, containing all the statements to perform is stored in the ASTNode's on_condition_success field.
 * If the condition fails and this if statement had an ELSE clause, the NODE_BLOCK, containing all the statements to perform is stored in the ASTNode's on_condition_failure field. Otherwise on_condition_failure is NULL'ed.
 */
ASTNode *parse_if_statement(ParserContext *ctx) {
	assert(ctx->cur_token->token_type == TOKEN_KEYWORD_IF);
	ASTNode* if_stmt = new_node_general(ctx, NODE_IF, ctx->cur_token);
	advance_token(ctx);

	if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
		|| 	ctx->cur_token->punc_type != PUNC_OPEN_PAREN	) {
		ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "open parenthesis '(' for if clause", true);
	}
	advance_token(ctx); // consume (
	
	if_stmt->condition = parse_expression(ctx); // parse expression inside condition
	// ie if we hade if (age == 18) { ... } then if_stmt->condition would be age == 18 as an AST

	if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
		|| 	ctx->cur_token->punc_type != PUNC_CLOSE_PAREN	) {
		ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "close parenthesis ')' for if clause", true);
	}
	advance_token(ctx); // consume )

	if_stmt->on_condition_success = parse_block(ctx);

	// DEBUG_TOKEN_STR(ctx->cur_token);
	// printf("%s\n", token_type_to_str(ctx->cur_token));
	if (ctx->cur_token->token_type == TOKEN_KEYWORD_ELSE) {
		advance_token(ctx);

		// else if
		if (ctx->cur_token->token_type == TOKEN_KEYWORD_IF) {
			if_stmt->on_condition_failure = parse_if_statement(ctx);
		}

		// just else
		else if_stmt->on_condition_failure = parse_block(ctx);
	}

	return if_stmt;
}

/*
 * Parses a while statement.
 * Returns a NODE_FOR node, the ASTNode's initial and increment fields are NULL'ed, the condition step is stored in the condition field. 
 * The NODE_BLOCK, containing all the statements to perform during the loop is stored in the ASTNode's body field.
 */
ASTNode *parse_while_statement(ParserContext *ctx) {
	assert(ctx->cur_token->token_type == TOKEN_KEYWORD_WHILE);
	ASTNode* while_stmt = new_node_general(ctx, NODE_FOR, ctx->cur_token);
	advance_token(ctx);

	if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
		|| 	ctx->cur_token->punc_type != PUNC_OPEN_PAREN	) {
		ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "open parenthesis '(' for while clause", true);
	}
	advance_token(ctx); // consume (

	while_stmt->condition = parse_expression(ctx);

	if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
		|| 	ctx->cur_token->punc_type != PUNC_CLOSE_PAREN	) {
		ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "closing parenthesis ')' for end of while clause", true);
	}
	advance_token(ctx); // consume )

	while_stmt->body = parse_block(ctx);

	return while_stmt;
}

/*
 * Parses a for statement.
 * A for statement can come in two forms:
 * 		for (...; ...; ...) 	or 		for (... in []..[])
 * This function accounts for both of them.
 * Returns a NODE_FOR node, the init step is stored in the ASTNode's initial field, the condition step is stored in the condition field, and the increment step is stored in increment field. 
 * The NODE_BLOCK, containing all the statements to perform during the loop is stored in the ASTNode's body field.
 */
ASTNode *parse_for_statement(ParserContext *ctx) {
	assert(ctx->cur_token->token_type == TOKEN_KEYWORD_FOR);
	ASTNode* for_stmt = new_node_general(ctx, NODE_FOR, ctx->cur_token);
	advance_token(ctx);

	if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
		|| 	ctx->cur_token->punc_type != PUNC_OPEN_PAREN	) {
		ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "open parenthesis '(' for for clause", true);
	}
	advance_token(ctx); // consume (

	// init step
	set_new_scope(ctx);

	bool need_semicolon = false;
	bool declared = false;			// dictates whether the variable in the init step is already declared (e.g., for (i = 0; ...; ...)) or not already declared (e.g., for (i32 i = 0; ...; ...))
	bool shorthand_for = false;		// dictates whether we are using normal for (i.e. for(;;)) or shorthand for (i.e. for (i32 i in a..b)) 

	// general for (i32 i = 0; ...; ...) case or similar
	// i.e. defining a variable in the init step of the for clause
	if (ctx->cur_token->token_type == TOKEN_PRIMITIVE_TYPE_SPECIFIER) {
		for_stmt->initial = parse_variable_declaration(ctx, false, &declared);
		need_semicolon = true; // we didnt consume semicolon in parse_varaible_declaration so we consume it now

		// checking whether this is a shorthand for loop
		if (!declared) {
#ifdef DEBUG
			printf("HERE: %s\n", token_type_to_str(ctx->cur_token));
			DEBUG_TOKEN_STR(ctx->cur_token);
#endif
			if (ctx->cur_token->token_type == TOKEN_KEYWORD_IN) {
				shorthand_for = true;
				need_semicolon = false;
			}
		}
	} 
	
	// for (i = 0; ...; ...) with predeclared i
	// whatever the SYMBOL_IDENTIFIER is, it must be a previously defined variable
	else if (ctx->cur_token->token_type == TOKEN_SYMBOL_IDENTIFIER) {
		for_stmt->initial = parse_expr_statement(ctx);
	}

	// for (; ...; ...) case, i.e. skips init
	else if (	ctx->cur_token->token_type == TOKEN_PUNCTUATOR
			&&	ctx->cur_token->punc_type == PUNC_SEMICOLON		) {
		for_stmt->initial = NULL;
		advance_token(ctx);
	}

	else ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "primitive type specifier (newly defined variable) or a predefined symbol identifier", true);

	if (need_semicolon) {
		if (!declared) {
			ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "an assignment in the initial step of the for loop", true);
		}
		
		if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
			|| 	ctx->cur_token->punc_type != PUNC_SEMICOLON	) {
			ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "semicolon ';' for condition step of for clause", true);
		}
		advance_token(ctx); // consume ;
	}

	if (!shorthand_for) {
		// traditional for loops of form: for (init; cond; incr) { ... }
		// we are currently						     ^^^^ here

		// for (... ;; ...) case, i.e. skips condition, relying on break, or infinite loop
		if (	ctx->cur_token->token_type == TOKEN_PUNCTUATOR 
			&& 	ctx->cur_token->punc_type == PUNC_SEMICOLON		) {
			for_stmt->condition = NULL;
		} else {
			for_stmt->condition = parse_expression(ctx);
		}

		if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
			|| 	ctx->cur_token->punc_type != PUNC_SEMICOLON	) {
			ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "semicolon ';' for increment step of for clause", true);
		}
		advance_token(ctx); // consume ;

		// for (... ; ... ;) case, i.e. skips increment
		if (	ctx->cur_token->token_type == TOKEN_PUNCTUATOR 
			&& 	ctx->cur_token->punc_type == PUNC_CLOSE_PAREN	) {
			for_stmt->increment = NULL;
		} else {
			for_stmt->increment = parse_expression(ctx);
		}

		if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
			|| 	ctx->cur_token->punc_type != PUNC_CLOSE_PAREN	) {
			ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "closing parenthesis ')' for end of for clause", true);
		}
		advance_token(ctx); // consume )

		for_stmt->body = parse_block(ctx);

		exit_scope(ctx);
		return for_stmt;
	} else {
		// shorthand for loops of form: for (u32 i in a..b) { ... }
		// we are currently						   ^^ here
		assert(ctx->cur_token->token_type == TOKEN_KEYWORD_IN); // just make sure, in case we break this logic later
		advance_token(ctx); // consume "in"

		// need to:
		// * set i = a
		// * set condition to while i <= b
		// * set increment to i++
		
		// set i = a
#ifdef DEBUG
		DEBUG_TOKEN_STR(ctx->cur_token);
		print_token_info(ctx->cur_token); printf("\n");
#endif
		ASTNode* a_value = parse_expression(ctx);
		for_stmt->initial->r_value = a_value;
	
		if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
			|| 	ctx->cur_token->punc_type != PUNC_DOTDOT	) {
			ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "'..' to demark ending for int value", true);
		}

		Token* dotdot_token = ctx->cur_token;
		advance_token(ctx); // consume ".."

		ASTNode* b_value = parse_expression(ctx);
		ASTNode* iterator = for_stmt->initial->l_value;

		// set up condition i <= b
		ASTNode* condition = PALLOCT(ctx->arena, ASTNode, 1);
		condition->node_type = NODE_VARIABLE;
		condition->variable_symbol = iterator->variable_symbol;
		condition->token = iterator->token;

		for_stmt->condition = new_node_binary(ctx, NODE_LE, condition, b_value, dotdot_token);

		// set increment to i++
		ASTNode* increment = PALLOCT(ctx->arena, ASTNode, 1);
		increment->node_type = NODE_VARIABLE;
		increment->variable_symbol = iterator->variable_symbol;
		increment->token = iterator->token;

		for_stmt->increment = new_node_unary(ctx, NODE_INCREMENT, increment, increment->token);

		if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
			|| 	ctx->cur_token->punc_type != PUNC_CLOSE_PAREN	) {
			ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "closing parenthesis ')' for end of for clause", true);
		}
		advance_token(ctx); // consume )

		for_stmt->body = parse_block(ctx);

		exit_scope(ctx);
		return for_stmt;
	}
}

/*
 * Parses a block. A block is any code which is surrounded by { ... }.
 * Returns a NODE_BLOCK node, with all substatements stored as a linked list in the ASTNode's body field.
 * This function also deals with scopes automatically. A new scope is created for each block, and left once the block finishes.
 */
ASTNode *parse_block(ParserContext *ctx) {
	if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
		|| 	ctx->cur_token->punc_type != PUNC_OPEN_CURLY	) {
		ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "open curly '{' for block", true);
	}

	ASTNode* block = new_node_general(ctx, NODE_BLOCK, ctx->cur_token);
	advance_token(ctx); // consume {

	if (	ctx->cur_token->token_type == TOKEN_PUNCTUATOR
		&&	ctx->cur_token->punc_type == PUNC_CLOSE_CURLY	) {
		// should warn about empty block
	}

	set_new_scope(ctx);

	while (		ctx->cur_token->token_type != TOKEN_EOF
			&&!(ctx->cur_token->token_type == TOKEN_PUNCTUATOR
			&&	ctx->cur_token->punc_type == PUNC_CLOSE_CURLY	)) {
		ASTNode* statement = parse_statement(ctx);
		
		// push statement onto end of block body linked list
		if (block->body == NULL) block->body = statement;
		else {
			ASTNode* end = block->body;
			for (; end->next != NULL; end = end->next);
			end->next = statement;
		}
	}

	if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
		|| 	ctx->cur_token->punc_type != PUNC_CLOSE_CURLY	) {
		ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "close curly '}' for block", true);
	}
	advance_token(ctx); // consume }
	
	exit_scope(ctx);
	return block;
}

ASTNode *parse_return_statement(ParserContext *ctx) {
	assert(ctx->cur_token->token_type == TOKEN_KEYWORD_RETURN);

	if (ctx->cur_function == NULL) {
		ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "function to house 'return' statement (stray return)", true);
	}

	Token* ref = ctx->cur_token;
	advance_token(ctx); // consume 'return'
	
	TypeInfo* expected_return_type = ctx->cur_function->function_return_type;
	assert(expected_return_type != NULL);

	ASTNode* ret = new_node_general(ctx, NODE_RETURN, ref);

	if (	ctx->cur_token->token_type == TOKEN_PUNCTUATOR 
		&& 	ctx->cur_token->punc_type == PUNC_SEMICOLON		) {
		
		if (expected_return_type->type != TYPE_VOID) {
			ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "expression following 'return' for non-void function", true);
		}

		advance_token(ctx);
		return ret;
	}

	ASTNode* res_expr = parse_expression(ctx);
	ret->l_value = res_expr;

	if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR
		|| 	ctx->cur_token->punc_type != PUNC_SEMICOLON		){
		ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "';' semicolon to end return statement", true);
	}
	advance_token(ctx);

	return ret;
}

/*
 * Parses an expression statement, i.e. any expression followed by a semicolon.
 * Returns a NODE_EXPR_STMT node, with the expression stored in the ASTNode's l_value field.
 */
ASTNode *parse_expr_statement(ParserContext *ctx) {
	Token* ref = ctx->cur_token;
	ASTNode* expr = parse_expression(ctx);

	if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR
		|| 	ctx->cur_token->punc_type != PUNC_SEMICOLON		){
		// printf("%s %s", token_type_to_str(ctx->cur_token->token_type), punc_to_str(ctx->cur_token->punc_type));
		ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "';' semicolon to end expression statement", true);
	}

	advance_token(ctx);
	ASTNode* statement = new_node_general(ctx, NODE_EXPR_STMT, ref);
	statement->l_value = expr;
	return statement;
}

// operator presedence (lowest to highest)

// name                                 symbols                 examples
// Assignment	                        =, +=, -=, *=, /=	    x = y + 2
// Logical OR	                        ||	                    a || b
// Logical AND	                        &&	                    a && b
// Bitwise OR							|
// Bitwise XOR							^
// Bitwise AND							&
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

/*
 * Parses an expression using recursive descent, and returns the subtree the expression generates. See operator presedence.
 */
ASTNode *parse_expression(ParserContext *ctx) {
	return parse_variable_assignment(ctx);
}

ASTNode *parse_variable_assignment(ParserContext *ctx) {
	ASTNode* left = parse_logical_or(ctx);

	if (	ctx->cur_token->token_type != TOKEN_EOF
		&& 	ctx->cur_token->token_type == TOKEN_PUNCTUATOR
		&& 	ctx->cur_token->punc_type  == PUNC_ASSIGNMENT	) {
		Token* ref = ctx->cur_token;
		advance_token(ctx);

		if (	left->node_type != NODE_VARIABLE		/*	i32 v = 10	 */
			&&	left->node_type != NODE_MEMBER			/*  man.age = 21 */
			&&	left->node_type != NODE_DEREF			/*  *p = 10		 */
			&&	left->node_type != NODE_INDEX	) {		/*	arr[i] = 10  */

			// this should call if you are doing something like 5 = 1 + 2
			// i.e. lval is not a variable or member
			ERR_SEMANTIC_CTX(ctx->cl_ctx, ctx->cur_token, "attempted to assign a non-assignable lvalue", true);
		}

		// here instead we do recursive call because assignment links right to left 
		// i.e. a = b = c   <====> a = (b = c)
		ASTNode* right = parse_variable_assignment(ctx);
		return new_node_binary(ctx, NODE_ASSIGN, left, right, ref);
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
		Token* ref = ctx->cur_token;
		advance_token(ctx);

		if (	left->node_type != NODE_VARIABLE
			&&	left->node_type != NODE_MEMBER		) {
			// this should call if you are doing something like 5 += 1
			// i.e. lval is not a variable or member
			ERR_SEMANTIC_CTX(ctx->cl_ctx, ctx->cur_token, "attempted to assign a non-assignable lvalue", true);
		}

		/*		
		 * 		NODE_ASSIGN
		 *		/		  \
		 *  lval	     optype
		 * 				/	   \
		 *			lval       rval
		 */ 
		
		// it wouldnt really make sense to have something like x += y = z, so we just skip to logical or instead of allowing rhs to also be assignment
		ASTNode* right = parse_logical_or(ctx);

		NodeType optype = NODE_NULL_EXPR;
		switch (ref->punc_type) {
			case PUNC_ADDEQ:	optype = NODE_ADD; break;
			case PUNC_SUBEQ:	optype = NODE_SUB; break;
			case PUNC_MULEQ:	optype = NODE_MUL; break;
			case PUNC_DIVEQ:	optype = NODE_DIV; break;
			case PUNC_MODEQ:	optype = NODE_MOD; break;
			case PUNC_ANDEQ:	optype = NODE_BITAND; break;
			case PUNC_OREQ:		optype = NODE_BITOR; break;
			case PUNC_XOREQ:	optype = NODE_BITXOR; break;
			case PUNC_RS_EQ:	optype = NODE_SHR; break;
			case PUNC_LS_EQ:	optype = NODE_SHL; break;
			default: ERR_GENERAL("Unreachable");
		}

		ASTNode* rhs = new_node_binary(ctx, optype, left, right, ref);
		return new_node_binary(ctx, NODE_ASSIGN, left, rhs, ref); 
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
		left = new_node_binary(ctx, NODE_LOGOR, left, right, ref);
		/* the new_node_binary part forms the tree, basically we have just done
		 *		 LOG_OR
		 *		/		\
		 *	  left     right													*/

		// then setting left to be this new formed tree, the new tree becomes the left side for the next iteration of the loop
	}

	return left;
}

ASTNode* parse_logical_and(ParserContext* ctx) {
	ASTNode* left = parse_bitwise_or(ctx);

	while (		ctx->cur_token->token_type != TOKEN_EOF
		  && 	ctx->cur_token->token_type == TOKEN_PUNCTUATOR
		  && 	ctx->cur_token->punc_type == PUNC_LOGICAL_AND	) {
		Token* ref = ctx->cur_token;
		advance_token(ctx);

		ASTNode* right = parse_bitwise_or(ctx);
		left = new_node_binary(ctx, NODE_LOGAND, left, right, ref);
	}

	return left;
}

ASTNode* parse_bitwise_or(ParserContext* ctx) {
	ASTNode* left = parse_bitwise_xor(ctx);

	while (		ctx->cur_token->token_type != TOKEN_EOF
		  && 	ctx->cur_token->token_type == TOKEN_PUNCTUATOR
		  && 	ctx->cur_token->punc_type == PUNC_BITWISE_OR	) {
		Token* ref = ctx->cur_token;
		advance_token(ctx);

		ASTNode* right = parse_bitwise_xor(ctx);
		left = new_node_binary(ctx, NODE_BITOR, left, right, ref);
	}

	return left;
}

ASTNode* parse_bitwise_xor(ParserContext* ctx) {
	ASTNode* left = parse_bitwise_and(ctx);

	while (		ctx->cur_token->token_type != TOKEN_EOF
		  && 	ctx->cur_token->token_type == TOKEN_PUNCTUATOR
		  && 	ctx->cur_token->punc_type == PUNC_BITWISE_XOR	) {
		Token* ref = ctx->cur_token;
		advance_token(ctx);

		ASTNode* right = parse_bitwise_and(ctx);
		left = new_node_binary(ctx, NODE_BITXOR, left, right, ref);
	}

	return left;
}

ASTNode* parse_bitwise_and(ParserContext* ctx) {
	ASTNode* left = parse_equality(ctx);

	while (		ctx->cur_token->token_type != TOKEN_EOF
		  && 	ctx->cur_token->token_type == TOKEN_PUNCTUATOR
		  && 	ctx->cur_token->punc_type == PUNC_AMPERSAND	) { 
			//								 ^^^^^^^^^^^^^^ this might get confused with address operator
		Token* ref = ctx->cur_token;
		advance_token(ctx);

		ASTNode* right = parse_equality(ctx);
		left = new_node_binary(ctx, NODE_BITAND, left, right, ref);
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
		left = (type == PUNC_EQUALITY) ? new_node_binary(ctx, NODE_EQ, left, right, ref)
									   : new_node_binary(ctx, NODE_NE, left, right, ref);
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
			case PUNC_LESSTHAN: left = new_node_binary(ctx, NODE_LT, left, right, ref); break;
			case PUNC_GREATER:  left = new_node_binary(ctx, NODE_GT, left, right, ref); break;
			case PUNC_GEQ: 		left = new_node_binary(ctx, NODE_GE, left, right, ref); break;
			case PUNC_LEQ: 		left = new_node_binary(ctx, NODE_LE, left, right, ref); break;
			default: ERR_GENERAL("Unreachable");
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
		left = (type == PUNC_LS) 	? new_node_binary(ctx, NODE_SHL, left, right, ref)
									: new_node_binary(ctx, NODE_SHR, left, right, ref);
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
		left = (type == PUNC_ADDITION) ? new_node_binary(ctx, NODE_ADD, left, right, ref)
									   : new_node_binary(ctx, NODE_SUB, left, right, ref);
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
			case PUNC_MULTIPLY: left = new_node_binary(ctx, NODE_MUL, left, right, ref); break;
			case PUNC_DIVIDE:   left = new_node_binary(ctx, NODE_DIV, left, right, ref); break;
			case PUNC_MOD: 		left = new_node_binary(ctx, NODE_MOD, left, right, ref); break;
			default: ERR_GENERAL("Unreachable");
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
		return new_node_binary(ctx, NODE_EXP, left, right, ref);
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
				case PUNC_LOGICAL_NOT: 	return new_node_unary(ctx, NODE_NOT, operand, ref);
				case PUNC_AMPERSAND: 	return new_node_unary(ctx, NODE_ADDR, operand, ref);
				case PUNC_BITWISE_NOT: 	return new_node_unary(ctx, NODE_BITNOT, operand, ref);
				case PUNC_SUBTRACTION: 	return new_node_unary(ctx, NODE_NEG, operand, ref);
				case PUNC_MULTIPLY: 	return new_node_unary(ctx, NODE_DEREF, operand, ref);
				default: ERR_HALT_CTX(ctx->cl_ctx, "Unreachable");
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
					ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "member identifier", true);
				}

				left = new_node_memidentifier(ctx, left, ctx->cur_token->lexeme, ref);
				advance_token(ctx);
				break;
			}

			case PUNC_OPEN_SQUARE: {
				advance_token(ctx);
				ASTNode* right = parse_expression(ctx);

				if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
					|| 	ctx->cur_token->punc_type != PUNC_CLOSE_SQUARE	) {
					ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "closing square bracket ']'" , true);
				}

				left = new_node_binary(ctx, NODE_INDEX, left, right, ref);
				advance_token(ctx);
				break;
			}

			case PUNC_OPEN_PAREN: {
				if (ctx->cur_function_call == NULL) {
					ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "function to call", true);
				}

				left = parse_function_call(ctx, &left);
				ctx->cur_function_call = NULL;
				break;
			}

			case PUNC_INCREMENT:
			case PUNC_DECREMENT: {
				advance_token(ctx);
				left = (type == PUNC_INCREMENT) ? new_node_unary(ctx, NODE_INCREMENT, left, ref)
												: new_node_unary(ctx, NODE_DECREMENT, left, ref);
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
			switch (ctx->cur_token->punc_type) {
				case PUNC_OPEN_PAREN: {
					advance_token(ctx);
					ASTNode* subexpr = parse_expression(ctx);

					if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
						|| 	ctx->cur_token->punc_type != PUNC_CLOSE_PAREN	) {
						ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "closed expression, closed by ')'" , true);
					}
					
					advance_token(ctx);
					return subexpr;
				}

				// the only reason you would get here is if you did something like x += y = 5
				// this operation doesn't really make sense
				case PUNC_ASSIGNMENT: ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "non-assignment expression after a compound assignment operator" , true);
				
				default: break;
			}

			break;
		}

		case TOKEN_STRING_LITERAL: {
            ASTNode* str_node = PALLOCT(ctx->arena, ASTNode, 1);
            str_node->node_type = NODE_LITERAL_STRING;
            str_node->token = ctx->cur_token;
            
            advance_token(ctx);
            return str_node;
        }
		
		case TOKEN_SYMBOL_IDENTIFIER: {
            ASTNode* var_node = PALLOCT(ctx->arena, ASTNode, 1);
			NodeType type = NODE_VARIABLE;

			Token* peek = ctx->cur_token->next;
			if (	peek != NULL 
				&& 	peek->token_type == TOKEN_PUNCTUATOR 
				&& 	peek->punc_type == PUNC_OPEN_PAREN		) {
				type = NODE_FUNCTION;
			}

            var_node->node_type = type;

			if (type == NODE_VARIABLE) {
				Symbol* sym = symbol_lookup(ctx->cur_scope, ctx->cur_token->lexeme);
				if (sym == NULL) ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "previously declared identifier in the scope", true);
				var_node->variable_symbol = sym;
			}

			else if (type == NODE_FUNCTION) {
				ASTNode* fun = function_lookup(ctx, ctx->cur_token->lexeme);
				if (fun == NULL) ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "previously declared function in the scope", true);
				ctx->cur_function_call = fun;
				var_node->function_name = fun->function_name;
				var_node->function_return_type = fun->function_return_type;
			}

            var_node->token = ctx->cur_token;
            
            advance_token(ctx);
            return var_node;
        }

		case TOKEN_INT_LITERAL:
		case TOKEN_FLOAT_LITERAL: {
			ASTNode* val = PALLOCT(ctx->arena, ASTNode, 1);
			val->node_type = ctx->cur_token->token_type == TOKEN_INT_LITERAL ? NODE_LITERAL_INT : NODE_LITERAL_FLOAT;

			val->token = ctx->cur_token;
			advance_token(ctx);
			return val;
		}

		case TOKEN_NULL: {
			ASTNode* null_val = PALLOCT(ctx->arena, ASTNode, 1);
			null_val->node_type = NODE_NULL_EXPR;
			advance_token(ctx);
			return null_val;
		}

		default: break;
	}
	
	ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "valid expression", true);
	return NULL;
}

ASTNode* parse_function_call(ParserContext* ctx, ASTNode** rest) {
	assert(	ctx->cur_token->token_type == TOKEN_PUNCTUATOR 
		&& 	ctx->cur_token->punc_type == PUNC_OPEN_PAREN		);
	assert(ctx->cur_function_call != NULL);

	ASTNode* cur_func_call = ctx->cur_function_call;
	ctx->cur_function_call = NULL;

	Token* ref = ctx->cur_token;
	advance_token(ctx);
	ASTNode* func_call = PALLOCT(ctx->arena, ASTNode, 1);
	func_call->l_value = *rest;
	func_call->node_type = NODE_FUNCTION_CALL;
	func_call->token = ref;
	func_call->function_to_call = cur_func_call; 

	ASTNode* args = NULL;
	size_t provided_arg_count = 0;
	if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
		|| 	ctx->cur_token->punc_type != PUNC_CLOSE_PAREN	) {
		for (bool more_args = true; more_args;) {
			provided_arg_count++;
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

	size_t expected_param_count = function_get_param_count(cur_func_call);
	// printf("exp -> %lu\nactual -> %lu\n", expected_param_count, provided_arg_count);

	if (provided_arg_count > expected_param_count) {
		ERR_SEMANTIC_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "provided too many function arguments in function call", true);
	}
	else if (provided_arg_count < expected_param_count) {
		ERR_SEMANTIC_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "provided too few function arguments in function call", true);
	}

	if (	ctx->cur_token->token_type != TOKEN_PUNCTUATOR 
		|| 	ctx->cur_token->punc_type != PUNC_CLOSE_PAREN	) {
		ERR_SYNTAX_CTX(ctx->cl_ctx, ctx->cur_token, /* expected a */ "')' to close function arguments", true);
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
		case NODE_BITAND:				return "BITWISE_AND (&)";
		case NODE_BITOR:				return "BITWISE_OR (|)";
		case NODE_BITXOR:				return "BITWISE_XOR (^)";
        case NODE_BITNOT:               return "BITWISE_NOT (~)";
		case NODE_NEG:					return "NEGATE (-)";
        case NODE_EQ:                   return "EQUAL (==)";
        case NODE_NE:                   return "NOT_EQUAL (!=)";
        case NODE_LT:                   return "LESS_THAN (<)";
        case NODE_GT:                   return "GREATER_EQUAL (>)";
        case NODE_LE:                   return "LESS_EQUAL (<=)";
        case NODE_GE:                   return "GREATER_EQUAL (>=)";
        case NODE_NOT:                  return "UNARY_NOT (!)";
        case NODE_ADDR:                 return "ADDRESS_OF (&)";
        case NODE_INCREMENT:            return "INCREMENT (++)";
        case NODE_DECREMENT:            return "DECREMENT (--)";
        case NODE_DEREF:                return "DEREFERENCE (*)";
        case NODE_INDEX:                return "ARRAY_INDEX []";
        case NODE_MEMBER:               return "STRUCT_MEMBER (. or ->)";
		case NODE_FUNCTION:				return "FUNCTION";
        case NODE_FUNCTION_CALL:        return "FUNCTION_CALL ()";
		case NODE_NULL_EXPR:			return "NULL";
		case NODE_EXPR_STMT:			return "EXPRESSION STMT";
		case NODE_FOR:					return "FOR";
		case NODE_IF:					return "IF";
		case NODE_BLOCK:				return "BLOCK";
		case NODE_PARAMETER:			return "PARAMETER";
		case NODE_RETURN:				return "RETURN";
        default:                        printf("%d ", type); return "UNKNOWN_NODE";
    }
}

void trace(ASTNode* head, size_t depth) {
	if (head == NULL) return;

	for (size_t i = 0; i < depth; i++) printf("  ");
	printf("%s", node_to_str(head->node_type));

	if (head->node_type == NODE_VARIABLE || head->node_type == NODE_MEMBER || head->node_type == NODE_PARAMETER)
		printf(" [\"%s\"]", head->variable_symbol->name);

	if (head->node_type == NODE_FUNCTION) {
		printf(" [\"%s\"]", head->function_name);
		printf(" | RETURNS");
		TypeInfo* t_info = head->function_return_type;
		printf(" [type=%s]", type_to_str(t_info->type, t_info->is_unsigned));
		if (t_info->type != TYPE_VOID) {
			printf(" [pdepth=%hu]", t_info->pointer_depth);
			if (t_info->pointer_depth > 0) printf(" [%s]", t_info->is_optional ? "nullable" : "nonnull");
		}
	}

	if (head->node_type == NODE_VARIABLE || head->node_type == NODE_PARAMETER) {
		TypeInfo* t_info = head->variable_symbol->typeinfo;
		printf(" [type=%s]", type_to_str(t_info->type, t_info->is_unsigned));
		printf(" [pdepth=%hu]", t_info->pointer_depth);
		if (t_info->pointer_depth > 0) printf(" [%s]", t_info->is_optional ? "nullable" : "nonnull");
		printf(" [vari=%lu]", head->variable_symbol->variable_identifier);
	}

	if (head->node_type == NODE_FOR) {
		printf(" [ \n");

		for (size_t i = 0; i < depth; i++) printf("  ");
		if (head->initial) {
			printf("* INIT:\n"); trace(head->initial, depth + 1);
		}
		else printf("* INIT: EMPTY\n");

		for (size_t i = 0; i < depth; i++) printf("  ");
		if (head->condition) {
			printf("* COND:\n"); trace(head->condition, depth + 1);
		}
		else printf("* COND: EMPTY\n");

		for (size_t i = 0; i < depth; i++) printf("  ");
		if (head->increment) {
			printf("* INCR:\n"); trace(head->increment, depth + 1);
		}
		else printf("* INCR: EMPTY\n");
		
		for (size_t i = 0; i < depth; i++) printf("  ");
		printf("]");
	}

	if (head->node_type == NODE_IF) {
		printf(" [ \n"); 
		for (size_t i = 0; i < depth; i++) printf("  ");
		printf("* COND:\n"); trace(head->condition, depth + 1); 
		for (size_t i = 0; i < depth; i++) printf("  ");
		printf("]\n");
		if (head->on_condition_success) {
			for (size_t i = 0; i < depth; i++) printf("  ");
			printf("THEN\n"); trace(head->on_condition_success, depth + 1);
		}
		if (head->on_condition_failure) {
			for (size_t i = 0; i < depth; i++) printf("  ");
			printf("ELSE\n"); trace(head->on_condition_failure, depth + 1);
		}
	}

	if (head->node_type == NODE_LITERAL_INT) printf(" [%lu]", head->token->int_val);
	if (head->node_type == NODE_LITERAL_FLOAT) printf(" [%Lf]", head->token->float_val);
	if (head->node_type == NODE_LITERAL_STRING) printf(" [\"%s\"]", head->token->str_val);
	printf("\n");

	if (head->l_value != NULL) trace(head->l_value, depth + 1);
	if (head->r_value != NULL) trace(head->r_value, depth + 1);
	if (head->body != NULL) trace(head->body, depth + 1);
	if (head->node_type == NODE_FOR) {
		for (size_t i = 0; i < depth; i++) printf("  ");
		printf("ENDFOR\n");
	}
	if (head->node_type == NODE_IF) {
		for (size_t i = 0; i < depth; i++) printf("  ");
		printf("ENDIF\n");
	}
	if (head->node_type == NODE_FUNCTION) {
		for (size_t i = 0; i < depth; i++) printf("  ");
		printf("ENDFUNCTION\n");
	}
	if (head->next != NULL) trace(head->next, depth);
}

#endif
