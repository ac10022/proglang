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

    /*
     * the offset from s0 (frame pointer);
     * for now the offset if 8 bytes as -4(s0) stores return address
     * and -8(s0) will store previous stack frame
     * initial offset may vary from risc-v's version (32bit or 64bit XLEN)
     * and also whether we need to save ra and s0 values at all
     */
    codegen_context->next_offset = -8;
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
            case (IR_ASSIGN): // global variable

                break;
            default:
                break;
        }
    }
}

bool is_variable(IROperand operand) {
    return operand.type == IROP_SYMBOL || operand.type == IROP_TEMP;
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
            case (IR_ADD):
            //case (IR_SUB):
            //case (IR_MUL):
            //case (IR_DIV):
            //case (IR_EXP):
            //case (IR_MOD):
            //case (IR_BITAND):
            //case (IR_BITOR):
            //case (IR_BITXOR):
            //case (IR_LOGOR):
            //case (IR_LOGAND):
            //case (IR_SHL):
            //case (IR_SHR):
            //case (IR_ADDR):
            //case (IR_BITNOT):
            case (IR_ASSIGN):
                IROperand ir_operands[3] = { ir_instruction->dest, ir_instruction->src1, ir_instruction->src2 };
                // we check whether the variable has a stack slot mapping or not by using find_slot
                // if it doesn't we will invoke add_slot
                for (int i = 0; i < 3; i++) {
                    if (!find_slot(codegen_context, ir_operands[i])) {
                        if (is_variable(ir_operands[i]))
                            add_slot(codegen_context, ir_operands[i]);
                    }
                }
                break;
            case (IR_PARAM):
                if (!find_slot(codegen_context, ir_instruction->dest)) {
                    if (is_variable(ir_instruction->dest))
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
    IRInstruction *current_instruction = *p_ir_instruction;
    for (;
        current_instruction != NULL && current_instruction->op != IR_END_FUNC;
        current_instruction = current_instruction->next
        ) {
        switch (current_instruction->op) {
            case IR_ADD:
                break;
            case IR_SUB:

                break;
            case IR_MUL:

                break;
            case IR_DIV:

                break;
            case IR_EXP:

                break;
            case IR_MOD:

                break;
            case IR_BITAND:

                break;
            case IR_BITOR:

                break;
            case IR_BITXOR:

                break;
            case IR_ASSIGN:

                break;
            case IR_LOGAND:

                break;
            case IR_LOGOR:

                break;
            case IR_NEG:

                break;
            case IR_SHL:

                break;
            case IR_SHR:

                break;
            case IR_DEREF:

                break;
            case IR_ADDR:

                break;
            case IR_BITNOT:

                break;
            case IR_LE:

                break;
            case IR_LT:

                break;
            case IR_EQ:

                break;
            case IR_GT:

                break;
            case IR_GE:

                break;
            case IR_NE:

                break;
            case IR_CALL:

                break;
            case IR_JUMP:

                break;
            case IR_PARAM:

                break;
            default:
                break;
        }
    }
    *p_ir_instruction = current_instruction;
}

void process_function_definition(CodegenContext *codegen_context, IRInstruction **p_ir_instruction) {
    IRInstruction *current_instruction = *p_ir_instruction; //*p_ir_instruction is IR_BEGIN_FUNC at the start
    int frame_size = allocate_slots(codegen_context, current_instruction);
    frame_size = (frame_size + 15) & ~15; // 16-byte alignment at function call boundary of a stack pointer
    emit_prologue(codegen_context, frame_size, current_instruction->dest.func_name);
    generate_code(codegen_context, p_ir_instruction);
    emit_epilogue(codegen_context, frame_size);
}

// creates new stack frame
void emit_prologue(CodegenContext *codegen_context, int frame_size, const char *function_name) {
    fprintf(codegen_context->out, ".global %s\n", function_name);

    // push stack frame on the stack by moving stack pointer by <frame_size> bytes.
    // in risc-v the stack grows towards lower addresses,
    // so if we target for other architectures we might have to
    // modify this instruction to move the stack pointer toward
    // higher addresses
    // TODO: move stack pointer up or down depending on the target architecture
    fprintf(codegen_context->out, "\taddi sp, sp, -%d\n", frame_size);

    // TODO: change size of ra dynamically (ie 8 bytes, when targeting rv64)
    fprintf(codegen_context->out, "\tsw ra, %d(sp)\n", frame_size - 4);

    // TODO: same with s0
    fprintf(codegen_context->out, "\tsw s0, %d(sp)\n", frame_size - 8);

    // TODO: same as with stack pointer, the current frame pointer
    //  may move down or up depending on the architecture
    fprintf(codegen_context->out, "\taddi s0, sp, %d\n", frame_size);
}

// removes the stack frame
// TODO: all todos in prologue emitter are applicable to epilogue emitter
void emit_epilogue(CodegenContext *codegen_context, int frame_size) {
    fprintf(codegen_context->out, "\tlw ra, %d(sp)\n", frame_size - 4);
    fprintf(codegen_context->out, "\tlw s0, %d(sp)\n", frame_size - 8);
    fprintf(codegen_context->out, "\taddi sp, sp, %d\n", frame_size);
    fprintf(codegen_context->out, "\tret\n\n");
}

void emit_load(CodegenContext *codegen_cotext, const char *reg, IROperand operand) {
    TODO("implement load instruction emitter");
}

void emit_store(CodegenContext *codegen_context, const char *reg, IROperand operand) {
    TODO("implement store instruction emitter");
}

void emit_add(CodegenContext *codegen_context, const char *dest, const char *src1, const char *src2) {
    TODO("implement add instruction emitter");
}

void emit_addi(CodegenContext *codegen_context, const char *dest, int val) {
    TODO("implement add immediate instruction emitter");
}
