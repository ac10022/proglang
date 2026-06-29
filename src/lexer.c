#include "lexer.h"
#include "../include/base.h"

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

char* L_FindStringEnd(char* pointer) {
    char* start = pointer;
    while (*pointer != '"') {
        if (CHR_IS_NEWLINE(*pointer) || *pointer == '\0') ERR_GENERAL("Unclosed string literal at %p", start);
        if (*pointer == '\\') pointer++;
        pointer++;
    }
    return pointer;
}

uint64_t L_ReadEscapedCharacter(char* pointer, char** end) {
    // octal case
    size_t i = 0;
    uint64_t out = 0;

    while ('0' <= *pointer && *pointer <= '7' && i < 3) {
        out = (out << 3) + (*pointer++ - '0');
        i++;
    }

    if (i > 0) {
        *end = pointer;
        return out;
    }

    out = 0;

    // hex case
    if (*pointer == 'x') {
        pointer++;
        if (!isxdigit((unsigned char)*pointer)) {
            ERR_GENERAL("Invalid hex escape character at %p", pointer);
        }

        size_t hex_digits = 0;
        while (isxdigit((unsigned char)*pointer)) {
            // to stop overflow
            if (hex_digits < 16) out = (out << 4) + L_HexToInt(*pointer);
            pointer++;
            hex_digits++;
        }

        // give a warning if an overflow could've happened
        if (hex_digits >= 16) {
            WARN_GENERAL("Hex digit overflow, ignoring superflous hex digits at %p", pointer);
        }

        *end = pointer;
        return out;
    }

    // special utf8 escape characters
    *end = pointer + 1;
    switch (*pointer) {
        case 'a': return '\a';
        case 'b': return '\b';
        case 't': return '\t';
        case 'n': return '\n';
        case 'v': return '\v';
        case 'f': return '\f';
        case 'r': return '\r';
        case 'e': return 27;
        default: return (unsigned char)*pointer;
    }

    ERR_GENERAL("Unreachable");
}

Token* L_ReadStringLiteral(FileInfo* source, char* pointer, uint64_t* line_num) {
    char* start = pointer;
    
    char* end = L_FindStringEnd(pointer + 1);
    char* buffer = calloc(1, end - pointer);
    int buf_index = 0;

    char* p2 = pointer + 1;
    while (p2 < end) {
        if (*p2 == '\\') buffer[buf_index++] = L_ReadEscapedCharacter(p2 + 1, &p2);
        else buffer[buf_index++] = *p2++; 
    }

    Token* tok = L_NewToken(TOKEN_STRING_LITERAL, start, end + 1, source, *line_num);
    tok->str_val = buffer;
    return tok;
}

uint8_t L_HexToInt(uint8_t hex_char) {
    if ('0' <= hex_char && hex_char <= '9') return hex_char - '0';
    if ('a' <= hex_char && hex_char <= 'f') return hex_char - 'a' + 10;
    if ('A' <= hex_char && hex_char <= 'F') return hex_char - 'A' + 10;
    ERR_GENERAL("Unreachable");
}

Token* L_ReadCharacterLiteral(FileInfo* source, char* pointer, uint64_t* line_num) {
    char* escaped_char = pointer + 1;
    if (*escaped_char == '\0') {
        ERR_GENERAL("Invalid char literal at %p", escaped_char);
    }

    uint64_t value = L_ReadEscapedCharacter(escaped_char + 1, &escaped_char);
    char* end = strchr(escaped_char, '\'');
    if (!end) {
        ERR_GENERAL("Unclosed char literal at %p", escaped_char);
    }

    Token* tok = L_NewToken(TOKEN_NUMBER, pointer, end + 1, source, *line_num);
    tok->int_val = value;
    return tok;
}

TokenType L_GetIdentifierType(char* pointer, size_t len) {
    if (len == 2 && strncmp(pointer, "fn", 2) == 0) return TOKEN_KEYWORD_FUNCTION;
    if (len == 2 && strncmp(pointer, "if", 2) == 0) return TOKEN_KEYWORD_IF;
    if (len == 5 && strncmp(pointer, "while", 5) == 0) return TOKEN_KEYWORD_WHILE;
    if (len == 3 && strncmp(pointer, "for", 3) == 0) return TOKEN_KEYWORD_FOR;
    if (len == 6 && strncmp(pointer, "return", 6) == 0) return TOKEN_KEYWORD_RETURN;
    return TOKEN_SYMBOL_IDENTIFIER;
}

bool L_IsLegalIdentiferStart(char c) {
    return isalpha(c) || c == '_';
}

bool L_IsLegalIdentiferTail(char c) {
    return isalnum(c) || c == '_';
}

size_t L_ReadIdentifier(char *start) {
    char *pointer = start;
    if (!L_IsLegalIdentiferStart(*pointer)) return 0;
    pointer++;
    while (L_IsLegalIdentiferTail(*pointer)) pointer++;
    return pointer - start;
}

size_t L_ReadPunctuator(char* start) {
    char *custom_punctuators[] = {
        // bit operators
        "<<=",
        ">>=",
        ">>",
        "<<",
        
        // (in)equality
        "==",
        "!=",
        "<=",
        ">=",
        
        // arithmetic
        "++",
        "--",
        "+=",
        "-=",
        "*=",
        "/=",
        "%=",

        // bitwise
        "&=",
        "|=",
        "^=",
        
        // logic
        "&&",
        "||",
    };

    for (size_t i = 0; i < sizeof(custom_punctuators) / sizeof(*custom_punctuators); i++)
    {
        if (STR_STARTS_WITH(start, custom_punctuators[i])) return strlen(custom_punctuators[i]);
    }

    return ispunct(*start) ? 1 : 0;
}

Token* L_Tokenize(FileInfo* source) {
    char* pointer = source->contents;
    // start from beginning of file

    Token head = {};
    Token* cur = &head;
    // start a linked list

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
            cur->int_val = (char)cur->int_val;
            pointer += cur->length;
            continue;
        }

        // open parenthesis
        if (*pointer == '(') {
            cur = cur->next = L_NewToken(TOKEN_OPEN_PAREN, pointer, pointer + 1, source, line_num);
            pointer++;
            continue;
        }

        // close parenthesis
        if (*pointer == ')') {
            cur = cur->next = L_NewToken(TOKEN_CLOSE_PAREN, pointer, pointer + 1, source, line_num);
            pointer++;
            continue;
        }

        // identifiers or keywords
        size_t identifer_len = L_ReadIdentifier(pointer);
        if (identifer_len > 0) {
            TokenType type = L_GetIdentifierType(pointer, identifer_len);
            cur = cur->next = L_NewToken(type, pointer, pointer + (int)identifer_len, source, line_num);
            pointer += identifer_len;
            continue;
        }

        // punctuators
        size_t punctuator_len = L_ReadPunctuator(pointer);
        if (punctuator_len > 0) {
            cur = cur->next = L_NewToken(TOKEN_PUNCTUATOR, pointer, pointer + (int)punctuator_len, source, line_num);
            pointer += punctuator_len;
            continue;
        }

        ERR_GENERAL("Invalid token at %p (character '%c')", pointer, *pointer);
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

size_t L_TokensCount(Token *head) {
	size_t count = 0;
	Token *current = head;
	while (current != NULL) {
		count++;
		current = current->next;
	}

	return count;
}