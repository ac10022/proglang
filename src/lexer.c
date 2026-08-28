#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
    {"..", PUNC_DOTDOT},
    {"->", PUNC_ARROW},

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
    {"?", PUNC_QUESTION_MARK},
    {"(", PUNC_OPEN_PAREN},
    {")", PUNC_CLOSE_PAREN},
    {"[", PUNC_OPEN_SQUARE},
    {"]", PUNC_CLOSE_SQUARE},
    {",", PUNC_COMMA},
    {".", PUNC_DOT},
    {"{", PUNC_OPEN_CURLY},
    {"}", PUNC_CLOSE_CURLY},
    {";", PUNC_SEMICOLON},
    {"&", PUNC_AMPERSAND},
    {"|", PUNC_BITWISE_OR},
    {"^", PUNC_BITWISE_XOR}
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

Token* new_token(
    TokenType token_type, 
    char* start_pointer, 
    char* end_pointer,
    LexerContext* l_ctx
) {
    Token* tok = PALLOCT(l_ctx->arena, Token, 1);
    tok->token_type = token_type;
    tok->location = start_pointer;
    tok->length = end_pointer - start_pointer;
    tok->source = l_ctx->cur_source;
    tok->line_number = l_ctx->cur_linenum;

    tok->punc_type = PUNC_INVALID;
    tok->int_val = (uint64_t)0;
    tok->float_val = (long double)0;
    tok->str_val = NULL;

    return tok;
}

Token* read_num_literal(LexerContext* l_ctx, char* pointer) {
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
            if (is_float || *(pointer + 1) == '.') break; // prevent things like 1..32 or 1.3.2 being classed as a float 
            pointer++;
            is_float = true;
        }
        else if (isalnum(*pointer)) {
            pointer++;
        }
        else break;
    }

    TokenType type = is_float ? TOKEN_FLOAT_LITERAL : TOKEN_INT_LITERAL;
    Token* tok = new_token(type, start, pointer, l_ctx);

    char* end; // dont actually need this for lexer but required for strtold and strtoull
    if (is_float) {
        tok->float_val = strtold(start, &end);
    } else {
        tok->int_val = strtoull(start, &end, 0);
    }

    return tok;
}

char* find_string_end(LexerContext* l_ctx, char* pointer, bool* terminated) {
    *terminated = true;
    while (*pointer != '"') {
        if (CHR_IS_NEWLINE(*pointer) || *pointer == '\0') {
            ERR_GENERAL_CTX(l_ctx->cl_ctx, "%s:%lu:\tUnclosed string literal.", l_ctx->cur_source->filepath, l_ctx->cur_linenum);
            *terminated = false;
            return pointer;
        }

        if (*pointer == '\\') {
            // prevent stepping over the terminator
            if (*(pointer + 1) == '\0' || CHR_IS_NEWLINE(*(pointer + 1))) {
                ERR_GENERAL_CTX(l_ctx->cl_ctx, "%s:%lu:\tUnclosed string literal.", l_ctx->cur_source->filepath, l_ctx->cur_linenum);
                *terminated = false;
                return pointer;
            }

            pointer++;
        }
        
        pointer++;
    }
    return pointer;
}

uint64_t read_escaped_char(LexerContext* l_ctx, char* pointer, char** end) {
    // prevent stepping over the terminator
    if (*pointer == '\0') {
        ERR_GENERAL_CTX(l_ctx->cl_ctx, "%s:%lu:\tUnterminated escape sequence.", l_ctx->cur_source->filepath, l_ctx->cur_linenum);
        *end = pointer;
        return (uint64_t)0;
    }

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
            ERR_GENERAL_CTX(l_ctx->cl_ctx, "%s:%lu:\tInvalid hex escape sequence.", l_ctx->cur_source->filepath, l_ctx->cur_linenum);
            *end = pointer;
            return (uint64_t)0;
        }

        size_t hex_digits = 0;
        while (isxdigit((unsigned char)*pointer)) {
            // to stop overflow
            if (hex_digits < 16) out = (out << 4) + hex_to_int(*pointer);
            pointer++;
            hex_digits++;
        }

        // give a warning if an overflow could've happened
        if (hex_digits >= 16) {
            WARN_CTX(l_ctx->cl_ctx, "%s:%lu:\tHex digit overflow, ignoring superflous hex digits.", l_ctx->cur_source->filepath, l_ctx->cur_linenum);
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

Token* read_string_literal(LexerContext* l_ctx, char* pointer) {
    char* start = pointer;
    
    bool terminated = false;
    char* end = find_string_end(l_ctx, pointer + 1, &terminated);
    char* buffer = PALLOCS(l_ctx->arena, end - pointer);
    int buf_index = 0;

    char* p2 = pointer + 1;
    while (p2 < end) {
        if (*p2 == '\\') buffer[buf_index++] = read_escaped_char(l_ctx, p2 + 1, &p2);
        else buffer[buf_index++] = *p2++; 
    }

    // don't consume new line
    char* tok_end = end + 1;
    if (!terminated) {
        tok_end = end;
        while (*tok_end != '\0' && !CHR_IS_NEWLINE(*tok_end)) tok_end++;
    }

    Token* tok = new_token(TOKEN_STRING_LITERAL, start, tok_end, l_ctx);
    tok->str_val = buffer;
    return tok;
}

uint8_t hex_to_int(uint8_t hex_char) {
    if ('0' <= hex_char && hex_char <= '9') return hex_char - '0';
    if ('a' <= hex_char && hex_char <= 'f') return hex_char - 'a' + 10;
    if ('A' <= hex_char && hex_char <= 'F') return hex_char - 'A' + 10;
    ERR_GENERAL("Unreachable");
}

Token* read_char_literal(LexerContext* l_ctx, char* pointer) {
    char* start_char = pointer + 1;
    if (*start_char == '\0' || CHR_IS_NEWLINE(*start_char) || *start_char == '\'') {
        ERR_GENERAL_CTX(l_ctx->cl_ctx, "%s:%lu:\tInvalid or empty char literal.", l_ctx->cur_source->filepath, l_ctx->cur_linenum);

        char* empty_end = (*start_char == '\'') ? start_char + 1 : start_char;
        Token* broken = new_token(TOKEN_INT_LITERAL, pointer, empty_end, l_ctx);
        broken->int_val = (uint64_t)0;
        return broken;
    }

    uint64_t value = 0;
    char* next = start_char;

    if (*start_char == '\\') {
        value = read_escaped_char(l_ctx, start_char + 1, &next);
    }
    else {
        value = (unsigned char)(*start_char);
        next = start_char + 1;
    }

    // keep track of unclosed literal up until new line then just forget about it
    char* end = next;
    while (*end != '\'' && *end != '\0' && !CHR_IS_NEWLINE(*end)) end++;

    if (*end != '\'') {
        ERR_GENERAL_CTX(l_ctx->cl_ctx, "%s:%lu:\tUnclosed char literal.", l_ctx->cur_source->filepath, l_ctx->cur_linenum);

        Token* broken = new_token(TOKEN_INT_LITERAL, pointer, end, l_ctx);
        broken->int_val = value;
        return broken;
    }

    Token* tok = new_token(TOKEN_INT_LITERAL, pointer, end + 1, l_ctx);
    tok->int_val = value;
    return tok;
}

TokenType get_identifier_type(char* pointer, size_t len, Type* type, bool* is_unsigned) {
    if (len == 2 && strncmp(pointer, "fn", 2) == 0) return TOKEN_KEYWORD_FUNCTION;
    if (len == 2 && strncmp(pointer, "if", 2) == 0) return TOKEN_KEYWORD_IF;
    if (len == 4 && strncmp(pointer, "else", 4) == 0) return TOKEN_KEYWORD_ELSE;
    if (len == 5 && strncmp(pointer, "while", 5) == 0) return TOKEN_KEYWORD_WHILE;
    if (len == 3 && strncmp(pointer, "for", 3) == 0) return TOKEN_KEYWORD_FOR;
    if (len == 6 && strncmp(pointer, "return", 6) == 0) return TOKEN_KEYWORD_RETURN;
    if (len == 5 && strncmp(pointer, "break", 5) == 0) return TOKEN_KEYWORD_BREAK;
    if (len == 8 && strncmp(pointer, "continue", 8) == 0) return TOKEN_KEYWORD_CONTINUE;
    if (len == 6 && strncmp(pointer, "import", 6) == 0) return TOKEN_KEYWORD_IMPORT;
    if (len == 2 && strncmp(pointer, "in", 2) == 0) return TOKEN_KEYWORD_IN;
    if (len == 4 && strncmp(pointer, "NULL", 4) == 0) return TOKEN_NULL;

    *is_unsigned = false;

    if (len == 4 && strncmp(pointer, "void", 4) == 0) {
        *type = TYPE_VOID;
        return TOKEN_PRIMITIVE_TYPE_SPECIFIER;
    } 

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

    if (len == 4 && strncmp(pointer, "char", 4) == 0) {
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

#define LEGAL_IDENTIFIER_START(c)       (isalpha(c) || (c) == '_')
#define LEGAL_IDENTIFIER_TAIL(c)        (isalnum(c) || (c) == '_')

size_t read_identifier(char *start) {
    char *pointer = start;
    if (!LEGAL_IDENTIFIER_START(*pointer)) return 0;
    pointer++;
    while (LEGAL_IDENTIFIER_TAIL(*pointer)) pointer++;
    return pointer - start;
}

Punctuator read_punctuator(char* start, char** end, size_t* length) {
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

size_t get_type_size(Type* type) {
    size_t total_types = sizeof(primitive_type_sizes) / sizeof(primitive_type_sizes[0]);

    for (size_t i = 0; i < total_types; i++)
    {
        if (*type == primitive_type_sizes[i].type) return primitive_type_sizes[i].size;
    }
    
    return 0;
}

Token* tokenize(FileInfo* source, CompilerContext* c_ctx) {
    // initialise lexer context
    LexerContext l_ctx = {
        .cur_source = source,
        .cl_ctx = c_ctx->cl_ctx,
        .cur_linenum = (uint64_t)1,
        .arena = c_ctx->arena,
    };

    // start from beginning of file
    char* pointer = source->contents;

    // start a linked list
    Token head = {};
    Token* cur = &head;

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
            l_ctx.cur_linenum++;
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
            cur = cur->next = read_num_literal(&l_ctx, pointer);
            pointer += cur->length; // advance the pointer
            continue;
        }

        // string literal
        if (*pointer == '"') {
            cur = cur->next = read_string_literal(&l_ctx, pointer);
            pointer += cur->length;
            continue;
        }

        // character literal
        if (*pointer == '\'') {
            cur = cur->next = read_char_literal(&l_ctx, pointer);
            cur->int_val = (char)cur->int_val;
            pointer += cur->length;
            continue;
        }

        // identifiers or keywords
        size_t identifier_len = read_identifier(pointer);
        if (identifier_len > 0) {
            Type primitive_type = 0;
            bool is_unsigned = false;

            TokenType token_type = get_identifier_type(pointer, identifier_len, &primitive_type, &is_unsigned);

            Token* new_tok = new_token(token_type, pointer, pointer + (int)identifier_len, &l_ctx);

            if (token_type == TOKEN_PRIMITIVE_TYPE_SPECIFIER) {
                new_tok->typeinfo = PALLOCT(l_ctx.arena, TypeInfo, 1);
                new_tok->typeinfo->is_unsigned = is_unsigned;
                new_tok->typeinfo->type = primitive_type;
                new_tok->typeinfo->size = get_type_size(&primitive_type);

                char* peek = pointer + identifier_len;

                // pointer work, calculate depth, and if it is optional
                while (*peek == '*') {
                    new_tok->typeinfo->pointer_depth++;
                    peek++;
                }

                if (new_tok->typeinfo->pointer_depth > 0 && *peek == '?') {
                    new_tok->typeinfo->is_optional = true;
                    peek++;
                }

                identifier_len = peek - pointer;
                new_tok->length = identifier_len;
            } else { // its a varaible identifer
				new_tok->lexeme = PALLOCS(l_ctx.arena, identifier_len + 1);
				memcpy(new_tok->lexeme, pointer, identifier_len);
				new_tok->lexeme[identifier_len] = '\0';
			}

            cur = cur->next = new_tok;
            pointer += identifier_len;
            continue;
        }

        // punctuators
        size_t punctuator_len = 0;
        Punctuator punc_type = read_punctuator(pointer, &pointer, &punctuator_len);
        if (punc_type != PUNC_INVALID && punctuator_len != 0) {
            Token* new_tok = new_token(TOKEN_PUNCTUATOR, pointer - (int)punctuator_len, pointer, &l_ctx);
            new_tok->punc_type = punc_type;
            cur = cur->next = new_tok;
            continue;
        }

        ERR_GENERAL_CTX(l_ctx.cl_ctx, "%s:%lu:\tInvalid token (character '%c').", l_ctx.cur_source->filepath, l_ctx.cur_linenum, *pointer);
        pointer++;
    }

    cur = cur->next = new_token(TOKEN_EOF, pointer, pointer, &l_ctx);
    return head.next;
}

Token* tokenize_file(char* filepath, CompilerContext* c_ctx) {
    FileInfo* info = new_fileinfo(c_ctx, filepath);
    if (!info) {
        ERR_HALT_CTX(c_ctx->cl_ctx, "Failed to read source file '%s'.", filepath);
        return NULL;
    }
    return tokenize(info, c_ctx);
}

size_t get_token_count(Token *head) {
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
        case PUNC_DOTDOT:       return "..";
        case PUNC_ARROW:        return "->";
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
        case PUNC_AMPERSAND:    return "&";
        case PUNC_BITWISE_OR:   return "|";
        case PUNC_BITWISE_XOR:  return "^";
        case PUNC_BITWISE_NOT:  return "~";
        case PUNC_ASSIGNMENT:   return "=";
        case PUNC_LOGICAL_AND:  return "&&";
        case PUNC_LOGICAL_OR:   return "||";
        case PUNC_LOGICAL_NOT:  return "!";
        case PUNC_QUESTION_MARK:return "?";
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
		case TOKEN_KEYWORD_FUNCTION:    return "KEYWORD_FUNCTION";
		case TOKEN_SYMBOL_IDENTIFIER:   return "SYMBOL_IDENTIFIER";
        case TOKEN_KEYWORD_IN:          return "IN";
		case TOKEN_KEYWORD_IF:          return "KEYWORD_IF";
        case TOKEN_KEYWORD_ELSE:        return "KEYWORD_ELSE";
		case TOKEN_KEYWORD_WHILE:       return "KEYWORD_WHILE";   
		case TOKEN_KEYWORD_FOR:         return "KEYWORD_FOR";
		case TOKEN_KEYWORD_RETURN:      return "KEYWORD_RETURN";   
		case TOKEN_STRING_LITERAL:      return "STRING_LITERAL";  
		case TOKEN_INT_LITERAL:         return "INT_LITERAL";     
		case TOKEN_FLOAT_LITERAL:       return "FLOAT_LITERAL";  
		case TOKEN_PUNCTUATOR:          return "PUNCTUATOR"; 
        case TOKEN_NULL:                return "NULL_TOKEN";
		case TOKEN_EOF:                 return "END OF FILE"; 

        case TOKEN_PRIMITIVE_TYPE_SPECIFIER:
			if (current->typeinfo != NULL) {
				return type_to_str(current->typeinfo->type, current->typeinfo->is_unsigned);
			} else {
				return "PRIMITIVE_TYPE_SPECIFIER (UNKNOWN)";
			}
        
        default:                        return "UNKNOWN TOKEN TYPE";
	}
}

void print_token_info(Token* current) {
    printf("%s %s %lu %Lf %s %s %lu %s %hu %s",
			token_type_to_str(current),
			current->lexeme == NULL ? "(null)" : current->lexeme,
    		current->int_val,
			current->float_val,  
    		current->str_val, 
    		current->source->filepath,     
    		current->line_number,
			current->punc_type != PUNC_INVALID ? punc_to_str(current->punc_type) : "(null)",
            current->token_type == TOKEN_PRIMITIVE_TYPE_SPECIFIER ?   current->typeinfo->pointer_depth : 0,
            current->token_type == TOKEN_PRIMITIVE_TYPE_SPECIFIER ?(current->typeinfo->is_optional ? "nullable" : "nonnull") : "N/A"
		);
}

void print_tokens(Token *tokens) {
	Token *current = tokens;
	while (current != NULL) {
		print_token_info(current);
		printf("\n");
		current = current->next;
	}
}
#endif

