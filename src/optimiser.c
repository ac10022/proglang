#include "optimiser.h"

IROperation node_to_irop(NodeType type) {
    switch (type) {
        case NODE_ADD:      return IR_ADD;
        case NODE_SUB:      return IR_SUB;
        case NODE_MUL:      return IR_MUL;
        case NODE_DIV:      return IR_DIV;
        case NODE_MOD:      return IR_MOD;
        case NODE_EXP:      return IR_EXP;
        case NODE_BITAND:   return IR_BITAND;
        case NODE_BITOR:    return IR_BITOR;
        case NODE_BITXOR:   return IR_BITXOR;
        case NODE_LOGAND:   return IR_LOGAND;
        case NODE_LOGOR:    return IR_LOGOR;
        default:            TODO("implement this irop");
    }
}

void initialise_optim_context(OptimiserContext* ctx) {
    ctx->instructions = NULL;
    ctx->temp_var_index = (size_t)0;
}

void push_instruction(OptimiserContext* ctx, IRInstruction* instruction) {
    if (!instruction) return;
    
    if (ctx) {
        if (ctx->instructions) {
            IRInstruction* end = ctx->instructions;
            for (; end->next != NULL; end = end->next);
            end->next = instruction;
        }
        else {
            ctx->instructions = instruction;
        }
    }
}

void emit(
    OptimiserContext* ctx, 
    IROperation op,
    IROperand d, 
    IROperand src1, 
    IROperand src2
) {
    IRInstruction* instruction = calloc(1, sizeof(IRInstruction));
    instruction->op = op;
    instruction->dest = d;
    instruction->src1 = src1;
    instruction->src2 = src2;
    instruction->next = NULL;
    // print_ir(instruction); printf("\n");

    push_instruction(ctx, instruction);
}

IRInstruction* ast_to_ir(ASTNode* root) {
    if (!root) ERR_GENERAL("Invalid AST provided for IR parsing");

    OptimiserContext o_ctx = {};
    initialise_optim_context(&o_ctx);

    for (ASTNode* statement = root; statement != NULL; statement = statement->next) {
        lower(&o_ctx, statement);
    }

    return o_ctx.instructions;
}

/*
 * the first stage is to convert ast -> ir, this process is called lowering
 * what is happening here, is we are just traversing the tree and calling lower on every node
 * every time there is an instruction which we should keep, then we emit it (emit()) to the ir linked list of instructions
 * 
 * for now, we do not do any optimisation while we finish the lowering process
 */

void lower(OptimiserContext* ctx, ASTNode* node) {
    if (!ctx) return;
    if (!node) return;

    switch (node->node_type) {
        case NODE_VARAIBLE_DECLARATION:
            // case with no initialiser (e.g., i32 x; )
            if (!node->r_value) break;
            
            // case with initialiser (e.g., i32 x = 10; )
            else {
                IROperand rhs = lower_expr(ctx, node->r_value);
                IROperand lhs = (IROperand) {
                    .type = IROP_SYMBOL,
                    .sym = node->l_value->variable_symbol,
                };

                emit(ctx, IR_ASSIGN, lhs, rhs, IROPERAND_EMPTY);
            }

            break;
        
        case NODE_IF:
        case NODE_FOR:
        case NODE_SWITCH:
        case NODE_RETURN:
        case NODE_BLOCK:
            TODO("lower statements");

        default:
            lower_expr(ctx, node);
            break;
    }
}

IROperand new_temp(OptimiserContext* ctx) {
    IROperand op = (IROperand) {
        .type = IROP_TEMP,
        .temp_id = ctx->temp_var_index,
    };
    ctx->temp_var_index++;
    return op;
}

IROperand lower_expr(OptimiserContext* ctx, ASTNode* node) {
    // printf("%s\n", node_to_str(node->node_type));
    switch (node->node_type) {
        case NODE_LITERAL_INT: 
            return (IROperand){
                .type = IROP_CONST_INT,
                .int_val = node->token->int_val,
            };

        case NODE_LITERAL_FLOAT:
            return (IROperand) {
                .type = IROP_CONST_FLOAT,
                .float_val = node->token->float_val,
            };

        case NODE_VARIABLE:
            return (IROperand) {
                .type = IROP_SYMBOL,
                .sym = node->variable_symbol,
            };

        case NODE_ASSIGN: {
            // lhs = [rhs expr]
            IROperand lhs = lower_expr(ctx, node->l_value); // dest
            IROperand rhs = lower_expr(ctx, node->r_value); // src1, src2 can just be IROPERAND_EMPTY
            emit(ctx, IR_ASSIGN, lhs, rhs, IROPERAND_EMPTY);
            return lhs;
        }

        // x = a || b || c
        // VARIABLE_DECLARATION
        //     VARIABLE ["x"] [type=BOOL] [pdepth=0] [vari=3]
        //     LOGICAL_OR (||)
        //         LOGICAL_OR (||)
        //             VARIABLE ["a"] [type=BOOL] [pdepth=0] [vari=0]
        //             VARIABLE ["b"] [type=BOOL] [pdepth=0] [vari=1]
        //         VARIABLE ["c"] [type=BOOL] [pdepth=0] [vari=2]
        // t0 = a || b
        // x = t0 || c

        case NODE_ADD:
        case NODE_SUB:
        case NODE_MUL:
        case NODE_DIV:
        case NODE_MOD:
        case NODE_EXP:
        case NODE_BITAND:
        case NODE_BITOR:
        case NODE_BITXOR: 
        case NODE_LOGOR:
        case NODE_LOGAND: {
            IROperand lhs = lower_expr(ctx, node->l_value);
            IROperand rhs = lower_expr(ctx, node->r_value);
            IROperand temp = new_temp(ctx);
            emit(ctx, node_to_irop(node->node_type), temp, lhs, rhs);
            return temp;
        }

        default: TODO("lower specific expression type");
    }
}

#ifdef DEBUG

const char* irop_to_str(IROperation op) {
    switch (op) {
        case IR_ADD:        return "+";
        case IR_SUB:        return "-";
        case IR_MUL:        return "*";
        case IR_DIV:        return "/";
        case IR_EXP:        return "**";
        case IR_MOD:        return "%";
        case IR_BITAND:     return "&";
        case IR_BITOR:      return "|";
        case IR_BITXOR:     return "^";
        case IR_ASSIGN:     return "=";
        case IR_LOGAND:     return "&&";
        case IR_LOGOR:      return "||";
        case IR_CALL:       return "CALL";
        case IR_JUMP:       return "JUMP";
        default:            return "UNKNOWN";
    }
} 

void print_ir_operand(IROperand op) {
    switch (op.type) {
        case IROP_TEMP:         printf("t%lu", op.temp_id); return;
        case IROP_SYMBOL:       printf("%s", op.sym->name); return;
        case IROP_CONST_INT:    printf("%lu", op.int_val); return;
        case IROP_CONST_FLOAT:  printf("%Lf", op.float_val); return;

        case IROP_EMPTY:
        default:                printf("UNKNOWN");
    }
}

void print_ir(IRInstruction* instruction) {
    if (!instruction) return;
    if (instruction->op == IR_ASSIGN) {
        print_ir_operand(instruction->dest);
        printf(" = ");
        print_ir_operand(instruction->src1);
    } else {
        print_ir_operand(instruction->dest);
        printf(" = ");
        print_ir_operand(instruction->src1);
        printf(" %s ", irop_to_str(instruction->op));
        print_ir_operand(instruction->src2);
    }
}

void print_ir_list(IRInstruction* instruction_list) {
    IRInstruction* end = instruction_list;
    for (; end != NULL; end = end->next) {
        print_ir(end); printf("\n");
    }
}

#endif