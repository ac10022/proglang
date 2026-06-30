#include "../include/proglang.h"

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

const char* type_to_str(Type type, bool is_unsigned) {
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

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: proglang <source file>\n");
		return EXIT_FAILURE;
	}

	char *file_path = argv[1];
	Token *tokens = L_TokenizeFile(file_path);

	Token *current = tokens;
	char token_type[256];
	while (current != NULL) {
		switch (current->type) {
			case TOKEN_KEYWORD_FUNCTION: 
				snprintf(token_type, 256, "KEYWORD_FUNCTION");
				break;
			case TOKEN_SYMBOL_IDENTIFIER:
				snprintf(token_type, 256, "SYMBOL_IDENTIFIER");
				break;
			case TOKEN_PRIMITIVE_TYPE_SPECIFIER:
                if (current->typeinfo != NULL) {
                    snprintf(token_type, 256, "PRIMITIVE_TYPE_SPECIFIER (%s: size=%hhu)", 
                             type_to_str(current->typeinfo->type, current->typeinfo->is_unsigned), current->typeinfo->size);
                } else {
                    snprintf(token_type, 256, "PRIMITIVE_TYPE_SPECIFIER (UNKNOWN)");
                }
                break;
			case TOKEN_KEYWORD_IF: 
				snprintf(token_type, 256, "KEYWORD_IF");
				break;
			case TOKEN_KEYWORD_WHILE: 
				snprintf(token_type, 256, "KEYWORD_WHILE");   
				break;
			case TOKEN_KEYWORD_FOR: 
				snprintf(token_type, 256, "KEYWORD_FOR");
				break;
			case TOKEN_KEYWORD_RETURN: 
				snprintf(token_type, 256, "KEYWORD_RETURN");   
				break;
			case TOKEN_STRING_LITERAL: 
				snprintf(token_type, 256, "STRING_LITERAL \"%s\"", current->str_val);  
				break;
			case TOKEN_INT_LITERAL:
				snprintf(token_type, 256, "INT_LITERAL (%lu)", current->int_val);     
				break;
			case TOKEN_FLOAT_LITERAL:
				snprintf(token_type, 256, "FLOAT_LITERAL (%Lf)", current->float_val);  
				break;
			case TOKEN_PUNCTUATOR: 
				snprintf(token_type, 256, "PUNCTUATOR '%s'", punc_to_str(current->punc_type)); 
				break;
			case TOKEN_EOF:
				snprintf(token_type, 256, "END OF FILE"); 
				break;
		}
		printf("%s ", token_type);
		printf("%p %lu %Lf %s %s %lu %d",
			(void*) current->next, 
    		current->int_val,
			current->float_val,  
    		current->str_val, 
    		current->source->filepath,     
    		current->line_number,
			current->punc_type
		);
		printf("\n");

		current = current->next;
	}
}
