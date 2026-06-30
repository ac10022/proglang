#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "file.h"

typedef enum {
    TOKEN_KEYWORD_FUNCTION,
    TOKEN_SYMBOL_IDENTIFIER, 
    TOKEN_PRIMITIVE_TYPE_SPECIFIER,

    TOKEN_KEYWORD_IF,
    TOKEN_KEYWORD_WHILE,
    TOKEN_KEYWORD_FOR,
    TOKEN_KEYWORD_RETURN,

    TOKEN_STRING_LITERAL,
    TOKEN_INT_LITERAL,
    TOKEN_FLOAT_LITERAL,
    
    TOKEN_PUNCTUATOR, 
    TOKEN_EOF,
} TokenType;

// i'm going to keep these basic types for now, we can extend it to arrays, classes, structs etc.
typedef enum {
    TYPE_VOID,
    TYPE_BOOL,          // b8     v there is a separate flag for signed/unsigned
    TYPE_INT8,          // i8  or u8
    TYPE_INT16,         // i16 or u16
    TYPE_INT32,         // i32 or u32
    TYPE_INT64,         // i64 or u64
    TYPE_FLOAT32,       // f32
    TYPE_FLOAT64,       // f64
} Type;

typedef enum {
    // bit operators
    PUNC_LS_EQ,         // <<=
    PUNC_RS_EQ,         // >>=
    PUNC_LS,            // <<
    PUNC_RS,            // >>
    
    // (in)equality
    PUNC_EQUALITY,      // ==
    PUNC_INEQAULITY,    // !=   
    PUNC_LEQ,           // <=
    PUNC_GEQ,           // >=
    PUNC_GREATER,       // >
    PUNC_LESSTHAN,      // <
        
    // arithmetic
    PUNC_ADDITION,      // +
    PUNC_SUBTRACTION,   // -
    PUNC_MULTIPLY,      // *
    PUNC_DIVIDE,        // /
    PUNC_MOD,           // %
    PUNC_POW,           // **
    PUNC_INCREMENT,     // ++
    PUNC_DECREMENT,     // --
    PUNC_ADDEQ,         // +=
    PUNC_SUBEQ,         // -=
    PUNC_MULEQ,         // *=
    PUNC_DIVEQ,         // /=
    PUNC_MODEQ,         // %=

    // bitwise
    PUNC_ANDEQ,         // &=
    PUNC_OREQ,          // |=
    PUNC_XOREQ,         // ^=   
    PUNC_BITWISE_NOT,   // ~       
        
    // logic
    PUNC_ASSIGNMENT,    // =
    PUNC_LOGICAL_AND,   // &&
    PUNC_LOGICAL_OR,    // ||
    PUNC_LOGICAL_NOT,   // !

    // symbols
    PUNC_OPEN_PAREN,    // (
    PUNC_CLOSE_PAREN,   // )
    PUNC_OPEN_SQUARE,   // [
    PUNC_CLOSE_SQUARE,  // ]
    PUNC_COMMA,         // ,
    PUNC_DOT,           // .
    PUNC_OPEN_CURLY,    // {
    PUNC_CLOSE_CURLY,   // }
    PUNC_SEMICOLON,     // ;

    PUNC_INVALID,       // not a valid punctuator
} Punctuator;

typedef struct {
    Type type;
    uint8_t size;           // value of sizeof(type)
    bool is_unsigned;
    // bool is_pointer;     // TODO i have no idea how though
} TypeInfo;

typedef struct Token Token;

struct Token {
    TokenType type;
    Token* next;            // during tokenisation, we form a linked list of tokens

    uint64_t int_val;       // if the token is an integer TOKEN_NUMBER, store the literal
    long double float_val;  // if the token is a float TOKEN_NUMBER, store the literal
    char* str_val;          // if the toekn is a TOKEN_STRING_LITERAL, store the literal
    Punctuator punc_type;   // if the token is a TOKEN_PUNCTUATOR, store the type

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

