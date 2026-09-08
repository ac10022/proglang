#include "proglang.h"

#include <unistd.h>

#include "codegen.h"
extern int optind;  	// optind from getoptcore
extern char *optarg; 	// optarg from getoptcore
extern int optopt;		// optopt from getoptcore

/*
 * TODO:
 *	* move IR output to codegen
 *	* output codegen output to assembler if CF_GENERATE_ASSEMBLY is not enabled
 */

void initialise_compiler_context(CompilerContext* ctx) {
	if (!ctx) return;
	ctx->flags = (uint64_t)0;
	ctx->filepath = NULL;
	ctx->outpath = NULL;
	ctx->arena = NEW_ARENA;
	ctx->cl_ctx = new_cleanup_context(ctx->arena);
}

void check_for_errors(CompilerContext* ctx) {
	if (has_error_notice(ctx->cl_ctx)) compilation_exit(ctx->cl_ctx, true);
}

void print_help(void) {
	printf(
		"Usage: proglang <source file> [options]\n"
		"\n"
		"Compiler options:\n"
	/* single character, boolean flags */
		"  -S\tGenerate assembly file instead of compiled binary.\n"
		"  -O\tUse optimisations.\n"
		"\n"
	/* string flags, i.e. user must specify a string */
		"  -o\tSpecify out file name.\n"
		"\n"
#ifdef DEBUG
	/* debug only flags */
		"  -a\t[DEBUG] Show AST after parsing.\n"
		"  -i\t[DEBUG] Show IR after optimising.\n"
#endif
		"\n"
	/* help flags */
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
	 *  -o			Specify out file name.
	 *
	 * 	(DEBUG only options)
	 * 	-a			[DEBUG] Show AST after parsing.
	 * 	-i			[DEBUG] Show IR instruction list after optimising.
	 */

#ifndef DEBUG
	char* optstring = "SOho:";
#else
	char* optstring = "SOaiho:";
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

			case 'o': {
				ctx.outpath = optarg;
				break;
			}

			default: {
				fprintf(stderr, "Usage: proglang <source file> [options: see -h]\n");
				ERR_HALT_CTX(ctx.cl_ctx, "Unknown argument '-%c'.", optopt);
			}
		}
	}

	assert(optind >= 0);
	if (optind >= argc) {
        fprintf(stderr, "Usage: proglang <source file> [options: see -h]\n");
        ERR_HALT_CTX(ctx.cl_ctx, "No input file provided.");
    }

	ctx.filepath = argv[optind];

	if (!check_file_exists(ctx.filepath)) {
		fprintf(stderr, "Usage: proglang <source file> [options: see -h]\n");
        ERR_HALT_CTX(ctx.cl_ctx, "Failed to read input file.");
	}

	if (!ctx.outpath) {
		ctx.outpath = replace_ext(ctx.arena, ctx.filepath, ".out");
		INFO_CTX(ctx.cl_ctx, "No output path specified, defaulting to '%s'.", ctx.outpath);
	}

	Token *tokens = tokenize_file(ctx.filepath, &ctx);
	check_for_errors(&ctx);

	ASTNode *ast = generate_ast(tokens, &ctx);
	check_for_errors(&ctx);

#ifdef DEBUG
	if (ctx.flags & CF_AST_TRACE) {
		printf("\n*** AST TRACE ***\n\n");
		trace(ast, 0);
	}
#endif

	OptimiserOutput optim_out = ast_to_ir(ast, &ctx);
	check_for_errors(&ctx);
	
#ifdef DEBUG
	if (ctx.flags & CF_IR_TRACE) {
		printf("\n*** IR LIST TRACE ***\n\n");
		print_string_list(optim_out.strings);
		printf("\n");
		print_ir_list(optim_out.instructions);
	}
#endif

	char* asm_filepath = NULL;
	generate_asm(optim_out, &ctx, &asm_filepath);
	check_for_errors(&ctx);

	INFO_CTX(ctx.cl_ctx, "Assembly output to '%s'", asm_filepath);

	compilation_exit(ctx.cl_ctx, false);
}
