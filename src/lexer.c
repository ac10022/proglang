#include "../include/lexer.h"
#include "../include/base.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

typedef struct {
    const char* symbol;
    Punctuator mapping;
} PunctuatorKVP;

// go from 3 char to 2 char to prevent exiting check too early
static const PunctuatorKVP custom_punctuators[] = {
    // 3 char
    {"<<=", PUNC_LS_EQ},
    {">>=", PUNC_RS_EQ},
    
    // 2 char
    {"<<", PUNC_LS},
    {">>", PUNC_RS},
    {"==", PUNC_EQUALITY},
    {"!=", PUNC_INEQAULITY},
    {"<=", PUNC_LEQ},
    {">=", PUNC_GEQ},
    {"**", PUNC_POW},
    {"++", PUNC_INCREMENT},
    {"--", PUNC_DECREMENT},
    {"+=", PUNC_ADDEQ},
    {"-=", PUNC_SUBEQ},
    {"*=", PUNC_MULEQ},
    {"/=", PUNC_DIVEQ},
    {"%=", PUNC_MODEQ},
    {"&=", PUNC_ANDEQ},
    {"|=", PUNC_OREQ},
    {"^=", PUNC_XOREQ},
    {"&&", PUNC_LOGICAL_AND},
    {"||", PUNC_LOGICAL_OR},

    // 1 char
    {">", PUNC_GREATER},
    {"<", PUNC_LESSTHAN},
    {"+", PUNC_ADDITION},
    {"-", PUNC_SUBTRACTION},
    {"*", PUNC_MULTIPLY},
    {"/", PUNC_DIVIDE},
    {"%", PUNC_MOD},
    {"~", PUNC_BITWISE_NOT},
    {"=", PUNC_ASSIGNMENT},
    {"!", PUNC_LOGICAL_NOT},
    {"(", PUNC_OPEN_PAREN},
    {")", PUNC_CLOSE_PAREN},
    {"[", PUNC_OPEN_SQUARE},
    {"]", PUNC_CLOSE_SQUARE},
    {",", PUNC_COMMA},
    {".", PUNC_DOT},
    {"{", PUNC_OPEN_CURLY},
    {"}", PUNC_CLOSE_CURLY},
    {";", PUNC_SEMICOLON}
};

typedef struct {
    Type type;
    uint8_t size;
} TypeSizeKVP;

static const TypeSizeKVP primitive_type_sizes[] = {
    {TYPE_BOOL, 8},
    {TYPE_INT8, 8},
    {TYPE_INT16, 16},
    {TYPE_INT32, 32},
    {TYPE_INT64, 64},
    {TYPE_FLOAT32, 32},
    {TYPE_FLOAT64, 64},
};

Token* L_NewToken(
    TokenType token_type, 
    char* start_pointer, 
    char* end_pointer,
    FileInfo* source,
    uint64_t line_number) 
{
    Token* tok = calloc(1, sizeof(Token));
    tok->token_type = token_type;
    tok->location = start_pointer;
    tok->length = end_pointer - start_pointer;
    tok->source = source;
    tok->line_number = line_number;

    return tok;
}

Token* L_ReadNumberLiteral(FileInfo* source, char* pointer, uint64_t* line_num) {
    char* start = pointer++;
    bool is_float = false;

    while (1) {
        // decimal scientific notation e.g., 1e+6 == 100_000
        // hex binary exponent, e.g., 0x1.8p3 == 1.5 * 2^3 = 12
        if ((*pointer) && (*(pointer + 1)) 
            && strchr("eEpP", *pointer) 
            && strchr("+-", *(pointer + 1))) {
                pointer += 2;
                is_float = true;
        }
        else if (*pointer == '.') {
            pointer++;
            is_float = true;
        }
        else if (isalnum(*pointer)) {
            pointer++;
        }
        else break;
    }

    TokenType type = is_float ? TOKEN_FLOAT_LITERAL : TOKEN_INT_LITERAL;
    Token* tok = L_NewToken(type, start, pointer, source, *line_num);

    char* end; // dont actually need this for lexer but required for strtold and strtoull
    if (is_float) {
        tok->float_val = strtold(start, &end);
    } else {
        tok->int_val = strtoull(start, &end, 0);
    }

    return tok;
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
    char* start_char = pointer + 1;
    if (*start_char == '\0' || *start_char == '\'') {
        ERR_GENERAL("Invalid or empty char literal at %p", start_char);
    }

    uint64_t value = 0;
    char* next = start_char;

    if (*start_char == '\\') {
        value = L_ReadEscapedCharacter(start_char + 1, &next);
    }
    else {
        value = (unsigned char)(*start_char);
        next = start_char + 1;
    }

    char* end = strchr(next, '\'');
    if (!end) {
        ERR_GENERAL("Unclosed char literal at %p", next);
    }

    Token* tok = L_NewToken(TOKEN_INT_LITERAL, pointer, end + 1, source, *line_num);
    tok->int_val = value;
    return tok;
}

TokenType L_GetIdentifierType(char* pointer, size_t len, Type* type, bool* is_unsigned) {
    if (len == 2 && strncmp(pointer, "fn", 2) == 0) return TOKEN_KEYWORD_FUNCTION;
    if (len == 2 && strncmp(pointer, "if", 2) == 0) return TOKEN_KEYWORD_IF;
    if (len == 5 && strncmp(pointer, "while", 5) == 0) return TOKEN_KEYWORD_WHILE;
    if (len == 3 && strncmp(pointer, "for", 3) == 0) return TOKEN_KEYWORD_FOR;
    if (len == 6 && strncmp(pointer, "return", 6) == 0) return TOKEN_KEYWORD_RETURN;

    *is_unsigned = false;

    if (len == 2 && strncmp(pointer, "b8", 2) == 0) {
        *type = TYPE_BOOL;
        return TOKEN_PRIMITIVE_TYPE_SPECIFIER;
    } 

    if (len == 2 && strncmp(pointer, "i8", 2) == 0) {
        *type = TYPE_INT8;
        return TOKEN_PRIMITIVE_TYPE_SPECIFIER;
    } 

    if (len == 2 && strncmp(pointer, "u8", 2) == 0) {
        *type = TYPE_INT8;
        *is_unsigned = true;
        return TOKEN_PRIMITIVE_TYPE_SPECIFIER;
    } 

    if (len == 3 && strncmp(pointer, "i16", 3) == 0) {
        *type = TYPE_INT16;
        return TOKEN_PRIMITIVE_TYPE_SPECIFIER;
    } 
    
    if (len == 3 && strncmp(pointer, "u16", 3) == 0) {
        *type = TYPE_INT16;
        *is_unsigned = true;
        return TOKEN_PRIMITIVE_TYPE_SPECIFIER;
    } 

    if (len == 3 && strncmp(pointer, "i32", 3) == 0) {
        *type = TYPE_INT32;
        return TOKEN_PRIMITIVE_TYPE_SPECIFIER;
    } 

    if (len == 3 && strncmp(pointer, "u32", 3) == 0) {
        *type = TYPE_INT32;
        *is_unsigned = true;
        return TOKEN_PRIMITIVE_TYPE_SPECIFIER;
    } 

    if (len == 3 && strncmp(pointer, "i64", 3) == 0) {
        *type = TYPE_INT64;
        return TOKEN_PRIMITIVE_TYPE_SPECIFIER;
    } 

    if (len == 3 && strncmp(pointer, "u64", 3) == 0) {
        *type = TYPE_INT64;
        *is_unsigned = true;
        return TOKEN_PRIMITIVE_TYPE_SPECIFIER;
    } 

    if (len == 3 && strncmp(pointer, "f32", 3) == 0) {
        *type = TYPE_FLOAT32;
        return TOKEN_PRIMITIVE_TYPE_SPECIFIER;
    } 

    if (len == 3 && strncmp(pointer, "f64", 3) == 0) {
        *type = TYPE_FLOAT64;
        return TOKEN_PRIMITIVE_TYPE_SPECIFIER;
    } 

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

Punctuator L_ReadPunctuator(char* start, char** end, size_t* length) {
    size_t total_punctuators = sizeof(custom_punctuators) / sizeof(custom_punctuators[0]);

    for (size_t i = 0; i < total_punctuators; i++) {
        if (STR_STARTS_WITH(start, custom_punctuators[i].symbol)) {
            *length = strlen(custom_punctuators[i].symbol);
            *end = start + (int)(*length);
            return custom_punctuators[i].mapping;
        }
    }

    *end = start;
    return PUNC_INVALID;
}

size_t L_GetTypeSize(Type* type) {
    size_t total_types = sizeof(primitive_type_sizes) / sizeof(primitive_type_sizes[0]);

    for (size_t i = 0; i < total_types; i++)
    {
        if (*type == primitive_type_sizes[i].type) return primitive_type_sizes[i].size;
    }
    
    return 0;
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

        // identifiers or keywords
        size_t identifier_len = L_ReadIdentifier(pointer);
        if (identifier_len > 0) {
            Type primitive_type = 0;
            bool is_unsigned = false;

            TokenType token_type = L_GetIdentifierType(pointer, identifier_len, &primitive_type, &is_unsigned);

            Token* new_tok = L_NewToken(token_type, pointer, pointer + (int)identifier_len, source, line_num);

            if (token_type == TOKEN_PRIMITIVE_TYPE_SPECIFIER) {
                new_tok->typeinfo = calloc(1, sizeof(TypeInfo));
                new_tok->typeinfo->is_unsigned = is_unsigned;
                new_tok->typeinfo->type = primitive_type;
                new_tok->typeinfo->size = L_GetTypeSize(&primitive_type);
            } else { // its a varaible identifer
				new_tok->lexeme = malloc(identifier_len + 1);
				memcpy(new_tok->lexeme, pointer, identifier_len);
				new_tok->lexeme[identifier_len] = '\0';
				pointer += identifier_len;
			}

            cur = cur->next = new_tok;

            pointer += identifier_len;
            continue;
        }

        // punctuators
        size_t punctuator_len = 0;
        Punctuator punc_type = L_ReadPunctuator(pointer, &pointer, &punctuator_len);
        if (punc_type != PUNC_INVALID && punctuator_len != 0) {
            Token* new_tok = L_NewToken(TOKEN_PUNCTUATOR, pointer - (int)punctuator_len, pointer, source, line_num);
            new_tok->punc_type = punc_type;
            cur = cur->next = new_tok;
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

#ifdef DEBUG
const char* punc_to_str(Punctuator punc) {
    switch (punc) {
        case PUNC_LS_EQ:        return "<<=";
        case PUNC_RS_EQ:        return ">>=";
        case PUNC_LS:           return "<<"; 
        case PUNC_RS:           return ">>"; 
        case PUNC_EQUALITY:     return "==";
        case PUNC_INEQAULITY:   return "!=";
        case PUNC_LEQ:          return "<=";
        case PUNC_GEQ:          return ">=";
        case PUNC_GREATER:      return ">";
        case PUNC_LESSTHAN:     return "<";
        case PUNC_ADDITION:     return "+";
        case PUNC_SUBTRACTION:  return "-";
        case PUNC_MULTIPLY:     return "*";
        case PUNC_DIVIDE:       return "/";
        case PUNC_MOD:          return "%";
        case PUNC_POW:          return "**";
        case PUNC_INCREMENT:    return "++";
        case PUNC_DECREMENT:    return "--";
        case PUNC_ADDEQ:        return "+=";
        case PUNC_SUBEQ:        return "-=";
        case PUNC_MULEQ:        return "*=";
        case PUNC_DIVEQ:        return "/=";
        case PUNC_MODEQ:        return "%=";
        case PUNC_ANDEQ:        return "&=";
        case PUNC_OREQ:         return "|=";
        case PUNC_XOREQ:        return "^=";
        case PUNC_BITWISE_NOT:  return "~";
        case PUNC_ASSIGNMENT:   return "=";
        case PUNC_LOGICAL_AND:  return "&&";
        case PUNC_LOGICAL_OR:   return "||";
        case PUNC_LOGICAL_NOT:  return "!";
        case PUNC_OPEN_PAREN:   return "(";
        case PUNC_CLOSE_PAREN:  return ")";
        case PUNC_OPEN_SQUARE:  return "[";
        case PUNC_CLOSE_SQUARE: return "]";
        case PUNC_COMMA:        return ",";
        case PUNC_DOT:          return ".";
        case PUNC_OPEN_CURLY:   return "{";
        case PUNC_CLOSE_CURLY:  return "}";
        case PUNC_SEMICOLON:    return ";";
        case PUNC_INVALID:      
        default:                return "INVALID";
    }
}

const char *type_to_str(Type type, bool is_unsigned) {
    switch (type) {
        case TYPE_VOID:    		return "VOID";
        case TYPE_BOOL:    		return "BOOL";
        case TYPE_INT8:    		return is_unsigned ? "UINT8" : "INT8";
        case TYPE_INT16:   		return is_unsigned ? "UINT16" : "INT16";
        case TYPE_INT32:   		return is_unsigned ? "UINT32" : "INT32";
        case TYPE_INT64:   		return is_unsigned ? "UINT64" : "INT64";
        case TYPE_FLOAT32: 		return "FLOAT32";
        case TYPE_FLOAT64: 		return "FLOAT64";
        default:           		return "UNKNOWN";
    }
}

const char *token_type_to_str(Token *current) {
	switch (current->token_type) {
		case TOKEN_KEYWORD_FUNCTION: 
			return "KEYWORD_FUNCTION";
			break;
		case TOKEN_SYMBOL_IDENTIFIER:
			return "SYMBOL_IDENTIFIER";
			break;
		case TOKEN_PRIMITIVE_TYPE_SPECIFIER:
			if (current->typeinfo != NULL) {
				return type_to_str(current->typeinfo->type, current->typeinfo->is_unsigned);
			} else {
				return "PRIMITIVE_TYPE_SPECIFIER (UNKNOWN)";
			}
			break;
		case TOKEN_KEYWORD_IF: 
			return "KEYWORD_IF";
			break;
		case TOKEN_KEYWORD_WHILE: 
			return "KEYWORD_WHILE";   
			break;
		case TOKEN_KEYWORD_FOR: 
			return "KEYWORD_FOR";
			break;
		case TOKEN_KEYWORD_RETURN: 
			return "KEYWORD_RETURN";   
			break;
		case TOKEN_STRING_LITERAL: 
			return "STRING_LITERAL";  
			break;
		case TOKEN_INT_LITERAL:
			return "INT_LITERAL";     
			break;
		case TOKEN_FLOAT_LITERAL:
			return "FLOAT_LITERAL";  
			break;
		case TOKEN_PUNCTUATOR: 
			return "PUNCTUATOR"; 
			break;
		case TOKEN_EOF:
			return "END OF FILE"; 
			break;
	}
}


void print_tokens(Token *tokens) {
	Token *current = tokens;
	while (current != NULL) {
		printf("%s %s %lu %Lf %s %s %lu %s",
			token_type_to_str(current),
			current->lexeme == NULL ? "(null)" : current->lexeme,
    		current->int_val,
			current->float_val,  
    		current->str_val, 
    		current->source->filepath,     
    		current->line_number,
			current->punc_type != NULL ? punc_to_str(current->punc_type) : "(null)"
		);
		printf("\n");

		current = current->next;
	}
}
#endif

