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

void initialise_codegen_context(
    CodegenContext *codegen_context,
    CompilerContext *compiler_context,
    FILE *out
) {
    codegen_context->out = out;
    codegen_context->compiler_context = compiler_context;
    codegen_context->frame_slot = NULL;
}

void generate_asm(IRInstruction *head, CompilerContext *c_ctx) {
    //char *outpath = create_asm_outpath(c_ctx->filepath);
    char *outpath = "a.s";
    FILE *out = fopen(outpath, "w");

    if (!out) {
        ERR_GENERAL_CTX(c_ctx->cl_ctx, "Failed to open '%s' for writing.", outpath);
        return;
    }

    CodegenContext codegen_context = {};
    initialise_codegen_context(&codegen_context, c_ctx, out);

    IRInstruction *current_instruction = head;
    for (; current_instruction->op != IR_HALT; current_instruction = current_instruction->next) {
        switch (current_instruction->op) {
            case (IR_BEGIN_FUNC):
                process_function_definition(&codegen_context, &current_instruction);
                break;

            default:
                break;
        }
    }
}

bool operand_equals(IROperand a, IROperand b) {
    if (a.type != b.type) return false;
    if (a.type == IROP_TEMP)   return a.temp_id == b.temp_id;
    if (a.type == IROP_SYMBOL) return a.sym == b.sym;
    return false;
}

FrameSlot *add_slot(CodegenContext *codegen_context, IROperand operand) {
    FrameSlot *frame_slot = calloc(1, sizeof(FrameSlot));
    frame_slot->operand = operand;

    //codegen_context->next_offset -= operand.sym->typeinfo->size;
    codegen_context->next_offset -= 4; // for now its juts 4 bytes (width of registers in RV32)
    frame_slot->offset = codegen_context->next_offset;

    frame_slot->next = codegen_context->frame_slot;
    codegen_context->frame_slot = frame_slot;

    return frame_slot;
}

FrameSlot *find_slot(const CodegenContext *codegen_context, IROperand operand) {
    for (FrameSlot* frame_slot = codegen_context->frame_slot; frame_slot != NULL; frame_slot = frame_slot->next) {
        if (operand_equals(frame_slot->operand, operand)) return frame_slot;
    }
    return NULL;
}

// this function goes through a function (i dont know how to rephrase it),
// and populates codegen_context->slots
int allocate_slots(CodegenContext *codegen_context, IRInstruction *ir_instruction) {
    int frame_size = 8; // could be optimized (for example, if the function
                        // doesn't call any other function then we dont need
                        // to store ra as it wont be altered anyways.
                        // but for now we always allocate some memory for
                        // previous frame pointer (4 bytes) and return address (4 bytes)

    // RISC-V calling convention (both RV32 and RV64) requires the stack pointer
    // to be 16-byte aligned at every function call boundary.
    // in other words frame_size has to be a multiple of 16
    for (; ir_instruction != NULL && ir_instruction->op != IR_END_FUNC; ir_instruction = ir_instruction->next) {
        switch (ir_instruction->op) {
            case (IR_ASSIGN):
                // we check whether the variable has a stack slot mapping or not by using find_slot
                // if it doesn't we will invoke add_slot
                if (!find_slot(codegen_context, ir_instruction->dest)) {
                    add_slot(codegen_context, ir_instruction->dest);
                }
                break;
            default:
                break;
        }
    }

    return frame_size;
}

void generate_code(CodegenContext *codegen_context, IRInstruction **p_ir_instruction) {
    for (
        IRInstruction *current_instruction = *p_ir_instruction;
        current_instruction != NULL && current_instruction->op != IR_HALT;
        current_instruction = current_instruction->next
        ) {
        switch (current_instruction->op) {
            // match and emit corresponding instruction
            default:
                break;
        }
    }
}

void process_function_definition(CodegenContext *codegen_context, IRInstruction **p_ir_instruction) {
    IRInstruction *current_instruction = *p_ir_instruction; //*p_ir_instruction is IR_BEGIN_FUNC at the start
    int frame_size = allocate_slots(codegen_context, current_instruction);
    emit_prologue(codegen_context, frame_size);
    generate_code(codegen_context, p_ir_instruction);
    emit_epilogue(codegen_context, frame_size);
}


// creates new stack frame
void emit_prologue(CodegenContext *codegen_context, int frame_size) {
    // fprint to out actual assembly instrucions
}

// removes the stack frame
void emit_epilogue(CodegenContext *codegen_context, int frame_size) {
    // fprint to out actual assembly instrucions
}

//void emit_load(CodegenContext *codegen_cotext, const char *reg, IROperand operand) {}
//void emit_store(CodegenContext *codegen_context, const char *reg, IROperand operand) {}
//
//void emit_add(CodegenContext *codegen_context, const char *dest, const char *src1, const char *src2) {}
//void emit_addi(CodegenContext *codegen_context, const char *dest, int val) {}
