#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "file.h"

typedef enum {
    TOKEN_KEYWORD_FUNCTION 	= 1,
    TOKEN_SYMBOL_IDENTIFIER = 2,

    TOKEN_KEYWORD_IF 		= 3,
    TOKEN_KEYWORD_WHILE 	= 4,
    TOKEN_KEYWORD_FOR 		= 5,
    TOKEN_KEYWORD_RETURN 	= 6,

    TOKEN_STRING_LITERAL 	= 7,
    TOKEN_NUMBER 			= 8,
    
    TOKEN_OPEN_PAREN 		= 9,
    TOKEN_CLOSE_PAREN 		= 10,
    TOKEN_PUNCTUATOR 		= 11,
    TOKEN_EOF 				= 12,
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

Token* L_ReadStringLiteral(FileInfo* source, char* pointer, uint64_t* line_num);
char* L_FindStringEnd(char* pointer);
uint8_t L_HexToInt(uint8_t hex_char);
Token *L_ReadCharacterLiteral(FileInfo *source, char *pointer, uint64_t *line_num);
uint64_t L_ReadEscapedCharacter(char* pointer, char** end);

bool L_IsLegalIdentiferStart(char c);
bool L_IsLegalIdentiferTail(char c);
size_t L_ReadIdentifier(char *start);

Token *L_Tokenize(FileInfo *source);
Token *L_TokenizeFile(char *filepath);
size_t L_TokensCount(Token *head);

#endif

