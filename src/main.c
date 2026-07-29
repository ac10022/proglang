#include "../include/proglang.h"

#include <unistd.h>
extern int optind; // optind from unistd

void initialise_compiler_context(CompilerContext* ctx) {
	if (!ctx) return;
	ctx->flags = (uint64_t)0;
	ctx->filepath = NULL;
	ctx->outpath = NULL;
}

void print_help(void) {
	printf(
		"Usage: proglang <source file> [options]\n"
		"\n"
		"Compiler options:\n"
		"  -S\tGenerate assembly file instead of compiled binary.\n"
		"  -O\tUse optimisations.\n"
#ifdef DEBUG
		"  -a\t[DEBUG] Show AST after parsing.\n"
		"  -i\t[DEBUG] Show IR after optimising.\n"
#endif
		"\n"
		"  -h\tShow help menu.\n"
	);
}

int main(int argc, char *argv[]) {
	CompilerContext ctx = {};
	initialise_compiler_context(&ctx);

	/*
	 * Compiler options:
	 *	-S			Generate assembly file instead of compiled binary.
	 *	-O			Use optimisations.
	 *  -h			Show help menu.
	 * 
	 * 	(DEBUG only options)
	 * 	-a			[DEBUG] Show AST after parsing.
	 * 	-i			[DEBUG] Show IR instruction list after optimising.
	 */

#ifndef DEBUG
	char* optstring = "SOh";
#else
	char* optstring = "SOaih";
#endif

	int opt = 0;
	while ((opt = getopt(argc, argv, optstring)) != -1) {
		switch (opt) {
			case 'S': ctx.flags |= CF_GENERATE_ASSEMBLY; break;
			case 'O': ctx.flags |= CF_USE_OPTIMISATIONS; break;

			// DEBUG only options
#ifdef DEBUG
			case 'a': ctx.flags |= CF_AST_TRACE; break;
			case 'i': ctx.flags |= CF_IR_TRACE; break;
#endif
			// end of DEBUG only options

			case 'h': {
				print_help();
				return EXIT_SUCCESS;
			}

			default: {
				fprintf(stderr, "Usage: proglang <source file> [options: see -h]\n");
				return EXIT_FAILURE;
			}
		}
	}

	if (optind >= argc) {
        fprintf(stderr, "Usage: proglang <source file> [options: see -h]\n");
        ERR_GENERAL("No input file provided.");
    }

	ctx.filepath = argv[optind];

	if (!check_file_exists(ctx.filepath)) {
		fprintf(stderr, "Usage: proglang <source file> [options: see -h]\n");
        ERR_GENERAL("Failed to read input file.");
	}

	Token *tokens = tokenize_file(ctx.filepath);

	ASTNode *ast = generate_ast(tokens);

#ifdef DEBUG
	if (ctx.flags & CF_AST_TRACE) {
		printf("\n*** AST TRACE ***\n\n");
		trace(ast, 0);
	}
#endif

	IRInstruction* ir_list = ast_to_ir(ast, &ctx);

#ifdef DEBUG
	if (ctx.flags & CF_IR_TRACE) {
		printf("\n*** IR LIST TRACE ***\n\n");
		print_ir_list(ir_list);
	}
#endif

}
