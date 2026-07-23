#ifndef OPTIMISER_H
#define OPTIMISER_H

#include "proglang.h"
#include "base.h"

typedef enum {
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_DIV,
    IR_EXP,
    IR_MOD,
    IR_BITAND,
    IR_BITOR,
    IR_BITXOR,
    IR_LOGOR,
    IR_LOGAND,
    IR_NEG,
    IR_SHL,
    IR_SHR,
    IR_DEREF,
    IR_ADDR,
    IR_BITNOT,
    IR_ASSIGN,
    IR_EQ,
    IR_NE,
    IR_LT,
    IR_LE,
    IR_GT,
    IR_GE,
    
    IR_CALL,
    IR_LABEL,
    IR_JUMP,
    IR_JUMP_IF_ZERO,
    IR_JUMP_IF_NONZERO,

    IR_BEGIN_SCOPE,
    IR_END_SCOPE,

    IR_HALT,
} IROperation;

typedef enum {
    IROP_EMPTY,
    IROP_SYMBOL,
    IROP_TEMP,
    IROP_CONST_INT,
    IROP_CONST_FLOAT,
    IROP_LABEL,
} IROperandType;

typedef struct {
    IROperandType type;
    union {
        Symbol* sym;
        size_t temp_id;
        size_t label_id;
        uint64_t int_val;
        long double float_val;
    };
} IROperand;

#define IROPERAND_EMPTY         (IROperand){ .type = IROP_EMPTY }
#define IROP_IS_EMPTY(irop)     ((irop).type == IROP_EMPTY)

#define TRUE                    (IROperand){ .type = IROP_CONST_INT, .int_val = 1 }
#define FALSE                   (IROperand){ .type = IROP_CONST_INT, .int_val = 0 }

typedef struct IRInstruction IRInstruction;

/*
 * the way IR representation is structured is like this:
 * lets say we translate i32 x = y + 10 * 5;
 * 
 * this would get translated to:
 * t0 = 10 * 5      // t0 is an IROP_TEMP (that is, a temporary variable)
 *      ^^   ^      // both literals are IROP_CONST_INT
 * x = y + t0
 * ^   ^            // both variables x, y are IROP_SYMBOLS
 */

struct IRInstruction {
    IROperation op;
    
    // every IR instructions is of the form
    // []       = []      op      []
    // ^^ dest    ^^ src1         ^^ src2   
    IROperand dest; 
    IROperand src1;
    IROperand src2;

    IRInstruction *next;
};

typedef struct {
    IRInstruction *instructions;
    size_t temp_var_index;          // this is a counter for when we declare new IROP_TEMPs
                                    // so that we always have a unique identifer
    
    size_t label_index;             // this is a counter for when we declare new IROP_LABELs
                                    // so that we always have a unique identifier for a label
} OptimiserContext;

IROperation node_to_irop(NodeType type);
void initialise_optim_context(OptimiserContext* ctx);
void push_instruction(OptimiserContext* ctx, IRInstruction* instruction);
void emit(OptimiserContext* ctx, IROperation op, IROperand d, IROperand src1, IROperand src2);
IRInstruction* ast_to_ir(ASTNode* root);

IROperand new_temp(OptimiserContext* ctx);
IROperand new_label(OptimiserContext* ctx);

void lower(OptimiserContext* ctx, ASTNode* node);
IROperand lower_expr(OptimiserContext* ctx, ASTNode* node);

#ifdef DEBUG
const char* irop_to_str(IROperation op);
void print_ir_operand(IROperand op);
void print_ir(IRInstruction* instruction);
void print_ir_list(IRInstruction* instruction_list);
#endif //debug

#endif