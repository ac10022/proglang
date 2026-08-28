#ifndef CODEGEN_H
#define CODEGEN_H

#include "optimiser.h"

// this is a naive implementation of code generator
// it only processes programs without global variables (only functions)
// for now it targets for RV32 version of risc-v, meaning size of the
// registers is 32 bits

/* TODO: target different RISC-V versions and only then different architectures
*
* TODO: process global variables (store on the stack), scopes,
*  and probably other stuff (research that)
*
* TODO: we can analyze how long variables live and generate
*  a more optimized assembly code. for example uninitialized variables,
*  unused variables, etc. can be avoided by the generator and therefore
*  reduce memory consumption
*
* TODO: other optimizations:
*  * could be calculating r-values during
*    compile-time, but this should be done during the AST buildng
*    or IR generation stages.
*  * instead of storing every variable in memory we can implement these:
*    graph-coloring register allocation, linear-scan register allocation
*/

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
 *   addi sp, sp, -16 # "grow" stack by 16 bytes (for example frame size is 16 bytes)
 *   sw   ra, 12(sp)  # save return address into stack slot with offset 12 (last 4 bytes of frame)
 *   sw   s0, 8(sp)  # save previous frame pointer into stack slot with offset 8 (second last 4 bytes of frame)
 *   addi s0, sp, 16  # calculates current frame and loads result into fp
 *
 *   li   t0, 10      # load immediate value 10 into t0
 *   sw   t0, -12(s0)  # store 10 into stack slot that represents 'x'
 *
 *   li a0, 0         # store 0 into return register (a0 and a1 are return registers)
 *
 *   lw   ra, 12(sp)  # restore previous return address
 *   lw   s0, 8(sp)  # restore previous frame pointer
 *   addi sp, sp, 16  # shrink the stack by 16 bytes, or pop the frame from the stack
 *
 *   ret
 * ----------------------------
 * NOTE: s0 can also be referred to as fp (frame pointer)
 */

// when pushing a frame, we need to allocate slots for variables, etc.
// and also assign the slots location to the symbols in the proglang program
typedef struct FrameSlot FrameSlot;
struct FrameSlot {
    IROperand operand;
    int offset;         // location of slots is determined by offsetting
                        // frame pointer by some number of bytes

    FrameSlot *next;    // next slot
};

typedef struct{
    CompilerContext *compiler_context;  // < immutable through out the whole code generation
    FILE *out;                          // <

    // change at each function definition
    FrameSlot *frame_slot;
    int next_offset;
} CodegenContext;

// TODO: i think we will hold our horses for this one
//enum Target {
//    RV32,
//    RV64,
//    X86_64,
//    ARM64
//};

char *create_asm_outpath(char *filepath);

void initialise_codegen_context(
    CodegenContext *codegen_context,
    CompilerContext *compiler_context,
    FILE *out
);

void generate_asm(IRInstruction* ir_instruction, CompilerContext* c_ctx);

bool is_variable(IROperand operand);
bool operand_equals(IROperand a, IROperand b);
FrameSlot *add_slot(CodegenContext *codegen_context, IROperand operand);
FrameSlot *find_slot(const CodegenContext *codegen_context, IROperand operand);

// returns the size of the frame in bytes
int allocate_slots(CodegenContext *codegen_context, IRInstruction *ir_instruction);

void generate_code(CodegenContext *codegen_context, IRInstruction **p_ir_instruction);

void process_function_definition(CodegenContext *codegen_context, IRInstruction **p_ir_instruction);

// creates new stack frame
void emit_prologue(CodegenContext *codegen_context, int frame_size, const char *function_name);
// removes the stack frame
void emit_epilogue(CodegenContext *codegen_context, int frame_size);

void emit_load(CodegenContext *codegen_cotext, const char *reg, IROperand operand);
void emit_load_immediate(CodegenContext *codegen_context, const char *reg, int immediate);

void emit_store(CodegenContext *codegen_context, const char *reg, IROperand operand);

void emit_add(CodegenContext *codegen_context, const char *dest, const char *src1, const char *src2);
void emit_addi(CodegenContext *codegen_context, const char *dest, int immediate);

#endif //CODEGEN_H
