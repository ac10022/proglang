#ifndef PARSER_H
#define PARSER_H

#include <string.h>
#include "lexer.h"

#define SCOPE_GLOBAL_DEPTH    0

typedef struct Symbol Symbol;
typedef struct Scope Scope;

struct Symbol {
    Symbol *next;
    char *name;
    TypeInfo *typeinfo;

#ifdef DEBUG
    size_t variable_identifier;
#endif
};

struct Scope {
    Scope *parent;
    uint8_t scope_depth;
    Symbol *symbols_head;
};

#include "ASTNode.h" // we should probably move all includes to proglang.h, this has to be here because of how the structs in this file are defined

typedef struct {
    Token *cur_token;
    ASTNode *cur_function;
    Scope *cur_scope;

#ifdef DEBUG
    size_t variable_counter; // im just using this as debug to track if we are referring back to the variables correctly
#endif
} ParserContext;

void initialise_parser_context(ParserContext *ctx, Token* head);
void advance_token(ParserContext *ctx);
void initalise_global_scope(ParserContext *ctx);

ASTNode *generate_ast(Token *head);

Scope *set_new_scope(ParserContext *ctx);
Scope *exit_scope(ParserContext *ctx);

Symbol *new_symbol(ParserContext *ctx, char *sym_identifier, TypeInfo* typeinfo);
Symbol *symbol_lookup(Scope *scope, char *sym_name);

ASTNode *new_node_general(NodeType type, Token *tok);
ASTNode *parse_variable_assignment(ParserContext *ctx);
ASTNode *parse_variable_declaration(ParserContext *ctx, bool expect_semicolon, bool* declared);
ASTNode *parse_statement(ParserContext *ctx);
ASTNode *parse_function(ParserContext *ctx);
ASTNode *parse_if_statement(ParserContext *ctx);
ASTNode *parse_while_statement(ParserContext *ctx);
ASTNode *parse_for_statement(ParserContext *ctx);
ASTNode *parse_block(ParserContext *ctx);
ASTNode *parse_return_statement(ParserContext *ctx);
ASTNode *parse_expr_statement(ParserContext *ctx);

ASTNode *new_node_binary(NodeType type, ASTNode *l_value, ASTNode *r_value, Token *tok);
ASTNode *new_node_unary(NodeType type, ASTNode *unary_operand, Token *tok);
ASTNode *new_node_memidentifier(ASTNode *l_value, char *identifier, Token *tok);

ASTNode *parse_expression(ParserContext *ctx);
ASTNode *parse_variable_assignment(ParserContext *ctx);
ASTNode *parse_logical_or(ParserContext *ctx);
ASTNode *parse_logical_and(ParserContext *ctx);
ASTNode* parse_bitwise_or(ParserContext* ctx);
ASTNode* parse_bitwise_xor(ParserContext* ctx);
ASTNode* parse_bitwise_and(ParserContext* ctx);
ASTNode *parse_equality(ParserContext *ctx);
ASTNode *parse_comparison(ParserContext *ctx);
ASTNode *parse_term(ParserContext *ctx);
ASTNode *parse_bitwise_shift(ParserContext *ctx);
ASTNode *parse_factor(ParserContext *ctx);
ASTNode *parse_exponentiation(ParserContext *ctx);
ASTNode *parse_unary(ParserContext *ctx);
ASTNode *parse_postfix(ParserContext *ctx);
ASTNode *parse_function_call(ParserContext *ctx, ASTNode **rest);
ASTNode *parse_else(ParserContext *ctx);

bool is_token_type(Token *token, TokenType type);

#ifdef DEBUG
const char* node_to_str(NodeType type);
void trace(ASTNode* head, size_t depth);
#endif

#endif