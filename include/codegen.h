#ifndef CODEGEN_H
#define CODEGEN_H

#include "optimiser.h"

// i think we will hold our horses for this one
enum Target {
    X86_64,
    ARM64
};

char *create_asm_outpath(char *filepath);
void generate_asm(IRInstruction* ir_instruction, CompilerContext* c_ctx);

#endif //CODEGEN_H
