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
	while (current != NULL) {
		printf("%u %p %lu %Lf %s %s %lu %s %u",
			current->type, 
			(void*) current->next, 
    		current->int_val,
			current->float_val,  
    		current->str_val, 
    		current->source->filepath,     
    		current->line_number,
    		current->location,   
    		current->length
		);    
	}
}
