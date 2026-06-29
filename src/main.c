#include <stdio.h>
#include <stdlib.h>
#include "../include/lexer.h"

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: proglang <source file>\n");
		return EXIT_FAILURE;
	}
	char *file_path = argv[1];
	Token *tokens = L_TokenizeFile(file_path);

	Token *current = tokens;
	char *token_type;
	while (current != NULL) {
		switch (current->type) {
			case 1: 
				token_type = "KEYWORD_FUNCTION";
				break;
			case 2:
				token_type = "SYMBOL_IDENTIFIER";
				break;
			case 3: 
				token_type = "KEYWORD_IF";
				break;
			case 4: 
				token_type = "KEYWORD_WHILE";   
				break;
			case 5: 
				token_type = "KEYWORD_FOR";
				break;
			case 6: 
				token_type = "KEYWORD_RETURN";   
				break;
			case 7: 
				token_type = "STRING_LITERAL";   
				break;
			case 8:
				token_type = "NUMBER";	   
				break;
			case 9:
				token_type = "OPEN_PAREN"; 	   
				break;
			case 10: 
				token_type = "CLOSE_PAREN";
				break;
			case 11: 
				token_type = "PUNCTUATOR";	   
				break;
			case 12:
				token_type = "EOF";
				break;
		}
		printf("%s ", token_type);
		printf("%p %lu %Lf %s %s %lu %s\n",
			(void*) current->next, 
    		current->int_val,
			current->float_val,  
    		current->str_val, 
    		current->source->filepath,     
    		current->line_number,
    		current->location
		);

		current = current->next;
	}
}
