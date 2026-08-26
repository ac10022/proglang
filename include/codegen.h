#ifndef CODEGEN_H
#define CODEGEN_H

#include "optimiser.h"

/*
 * by the end of code generation, we have to end up with
 * an assembly file (.s)
 *
 * let's take this for example:
 * ----------------------------
 *     int main() {
 *         int x = 10;
 *         return 0;
 *     }
 * ----------------------------
 * our generator should spit out something like:
 * ----------------------------
 * main:
 *   addi sp, sp, -32 # "grow" stack by 32 bytes (for example frame size is 32 bytes)
 *   sd   ra, 24(sp)  # save return address into stack slot with offset 24 (last 8 bytes of frame)
 *   sd   s0, 16(sp)  # save previous frame pointer into stack slot with offset 16 (second last 8 bytes of frame)
 *   addi s0, sp, 32  # load current frame pointer into frame pointer register
 * ----------------------------
 * NOTE: s0 can also be referred to as fp (frame pointer)
 */

// when allocating slots, we create a one-to-one mappings
// between variables or symbols (temps or t's as well) and
// and the stack slots
typedef struct {
    IROperand operand;
    int offset;

} StackSlot;

typedef struct{
    FILE *out;

} CodegenContext;

// i think we will hold our horses for this one
enum Target {
    X86_64,
    ARM64
};

char *create_asm_outpath(char *filepath);

void initialise_codegen_context(CodegenContext *codegen_context, FILE *out);
void generate_asm(IRInstruction* ir_instruction, CompilerContext* c_ctx);

// creates new stack frame
void emit_prologue(CodegenContext *codegen_context);

// removes the stack frame
void emit_epilogue(CodegenContext *codegen_context);

void emit_load(CodegenContext *codegen_cotext);
void emit_store(CodegenContext *codegen_context);

void emit_add(CodegenContext *codegen_context);
void emit_addi(CodegenContext *codegen_context);

#endif //CODEGEN_H
