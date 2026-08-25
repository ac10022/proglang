#include "codegen.h"

/*
 * TODO: the function has to extract the name of .s file, i.e: main.proglang -> main.s
 *
 *
 */
char *create_asm_outpath(char *filepath) {
    char *outpath = "";

    return outpath;
}

void generate_asm(IRInstruction *ir_instruction, CompilerContext *c_ctx) {
    char *outpath = create_asm_outpath(c_ctx->filepath);
    FILE *out = fopen(outpath, "w");

    if (!out) {
        ERR_GENERAL_CTX(c_ctx->cl_ctx, "Failed to open '%s' for writing.", outpath);
        return;
    }


}
