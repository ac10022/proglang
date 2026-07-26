#include "../include/proglang.h"

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: proglang <source file>\n");
		return EXIT_FAILURE;
	}

	char *file_path = argv[1];
	Token *tokens = tokenize_file(file_path);

#ifdef DEBUG
	// print_tokens(tokens);
#endif

	ASTNode *ast = generate_ast(tokens);

#ifdef DEBUG
	printf("\n*** AST TRACE ***\n\n");
	trace(ast, 0);
#endif
	IRInstruction* ir_list = ast_to_ir(ast);

#ifdef DEBUG
	printf("\n*** IR LIST TRACE ***\n\n");
	print_ir_list(ir_list);
#endif

}
