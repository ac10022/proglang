#include "../include/proglang.h"
#ifdef DEBUG
	#include "helpers.c"
#endif

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: proglang <source file>\n");
		return EXIT_FAILURE;
	}

	char *file_path = argv[1];
	Token *tokens = L_TokenizeFile(file_path);

#ifdef DEBUG
	printf("DEBUG\n");
	print_tokens(tokens);
#endif

}
