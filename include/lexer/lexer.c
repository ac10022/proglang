#include "lexer.h"
#include "../base/base.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

Token* L_NewToken(
    TokenType type, 
    char* start_pointer, 
    char* end_pointer,
    FileInfo* source,
    uint64_t line_number
) {
    Token* tok = calloc(1, sizeof(Token));

    tok->type = type;
    tok->location = start_pointer;
    tok->length = end_pointer - start_pointer;
    tok->source = source;
    tok->line_number = line_number;

    return tok;
}

Token* L_ReadNumberLiteral(FileInfo* source, char* pointer, uint64_t* line_num) {
    char* start = pointer++;

    while (1) {
        // decimal scientific notation e.g., 1e+6 == 100_000
        // hex binary exponent, e.g., 0x1.8p3 == 1.5 * 2^3 = 12
        // ^^ ive never seen this before(?) but we have to check for it
        if ((*pointer) && (*(pointer + 1)) 
            && strchr("eEpP", *pointer) 
            && strchr("+-", *(pointer + 1))) {
                pointer += 2;
        }
        
        else if (isalnum(*pointer) || *pointer == '.') {
            pointer++;
        }
        
        else break;
    }

    return L_NewToken(TOKEN_NUMBER, start, pointer, source, *line_num);
}

Token* L_ReadStringLiteral(FileInfo* source, char* pointer, uint64_t* line_num) {
    TODO("string literal");
}

Token* L_ReadCharacterLiteral(FileInfo* source, char* pointer, uint64_t* line_num) {
    TODO("character literal");
}

Token* L_Tokenize(FileInfo* source) {
    char* pointer = source->contents; // start from beginning of file

    Token head = {};
    Token* cur = &head; // start a linked list

    uint64_t line_num = 1;

    while (*pointer) {
        // skip comments
        if (STR_STARTS_WITH(pointer, "//")) {
            pointer += 2;
            while (*pointer && !CHR_IS_NEWLINE(*pointer)) {
                pointer++;
            }
            continue;
        }

        // skip newline
        if (CHR_IS_NEWLINE(*pointer)) {
            line_num++;
            pointer++;
            continue;
        }

        // skip whitespace
        if (isspace(*pointer)) {
            pointer++;
            continue;
        }

        // check for numeric type token
        if (isdigit(*pointer) || (*pointer == '.' && isdigit(*(pointer + 1)))) {
            cur = cur->next = L_ReadNumberLiteral(source, pointer, &line_num);
            pointer += cur->length; // advance the pointer
            continue;
        }

        // string literal
        if (*pointer == '"') {
            cur = cur->next = L_ReadStringLiteral(source, pointer, &line_num);
            pointer += cur->length;
            continue;
        }

        // character literal
        if (*pointer == '\'') {
            cur = cur->next = L_ReadCharacterLiteral(source, pointer, &line_num);
            pointer += cur->length;
            continue;
        }

        TODO("other token types, e.g., function identifier, open and close parenthesis");
    }

    cur = cur->next = L_NewToken(TOKEN_EOF, pointer, pointer, source, line_num);
    return head.next;
}

Token* L_TokenizeFile(char* filepath) {
    FileInfo* info = F_NewFileInfo(filepath);
    if (!info) {
        ERR_GENERAL("Failed to tokenize file.");
        return NULL;
    }
    return L_Tokenize(info);
}

// function factorial(int n)
// Token: "function", Type: KEYWORD_FUNCTION  
// Token: "factorial", Type: SYMBOL_IDENTIFIER  
// Token: "(", Type: SYMBOL_OPEN_PARENTHESIS  
// Token: "int", Type: SYMBOL_IDENTIFIER  
// Token: "n", Type: SYMBOL_IDENTIFIER  
// Token: ")", Type: SYMBOL_CLOSING_PARENTHESIS  
