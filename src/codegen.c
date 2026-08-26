#include "codegen.h"

/*
 * TODO: the function has to extract the name of .s file, i.e: main.proglang -> main.s
 *
 * i dont know whether output name has to match outpath or filepath,
 * but i think its irrelevant for now as at the end, assembly and object files
 * will be most likely deleted after program compilation
 *
 */
char *create_asm_outpath(char *filepath) {
    char *outpath = "";

    return outpath;
}

void initialise_codegen_context(CodegenContext *codegen_context, FILE *out) {
    codegen_context->out = out;

}

void generate_asm(IRInstruction *ir_instruction, CompilerContext *c_ctx) {
    //char *outpath = create_asm_outpath(c_ctx->filepath);
    char *outpath = "a.s";
    FILE *out = fopen(outpath, "w");

    if (!out) {
        ERR_GENERAL_CTX(c_ctx->cl_ctx, "Failed to open '%s' for writing.", outpath);
        return;
    }

    CodegenContext codegen_context = {};
    initialise_codegen_context(&codegen_context, out);

    for (; ir_instruction->op != IR_HALT; ir_instruction = ir_instruction->next) {

    }

}
