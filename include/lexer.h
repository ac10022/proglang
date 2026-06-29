#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "file.h"

typedef enum {
    TOKEN_KEYWORD_FUNCTION,
    TOKEN_SYMBOL_IDENTIFIER,
    TOKEN_STRING_LITERAL,
    TOKEN_NUMBER,
    TOKEN_OPEN_PAREN,
    TOKEN_CLOSE_PAREN,
    TOKEN_EOF
} TokenType;

// i'm going to keep these basic types for now, we can extend it to arrays, classes, structs etc.
typedef enum {
    TYPE_VOID,
    TYPE_BOOL,
    TYPE_CHAR,
    TYPE_INT8,
    TYPE_INT16,
    TYPE_INT32,
    TYPE_INT64,
    TYPE_FLOAT32,
    TYPE_FLOAT64,
    TYPE_POINTER,
} Type;

typedef struct {
    Type type;
    uint8_t size;           // value of sizeof(type)
    bool is_unsigned;
} TypeInfo;

typedef struct Token Token;

struct Token {
    TokenType type;
    Token* next;            // during tokenisation, we form a linked list of tokens

    uint64_t int_val;       // if the token is an integer TOKEN_NUMBER, store the literal
    long double float_val;  // if the token is a float TOKEN_NUMBER, store the literal
    char* str_val;          // if the toekn is a TOKEN_STRING_LITERAL, store the literal

    TypeInfo* typeinfo;
    FileInfo* source;       // the file which we found the token in
    uint64_t line_number;   // line number in that file
    char* location;         // pointer location
    uint16_t length;        // length of token (as a string)
};

Token *L_NewToken(TokenType type, char *start_pointer, char *end_pointer, FileInfo *source, uint64_t line_number);
Token *L_ReadNumberLiteral(FileInfo *source, char *pointer, uint64_t *line_num);
Token *L_ReadStringLiteral(FileInfo *source, char *pointer, uint64_t *line_num);
Token *L_ReadCharacterLiteral(FileInfo *source, char *pointer, uint64_t *line_num);
Token *L_Tokenize(FileInfo *source);
Token *L_TokenizeFile(char *filepath);
size_t L_TokensCount(Token *head);

#endif

