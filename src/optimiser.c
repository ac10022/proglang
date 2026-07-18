#include "optimiser.h"
#include "base.h"

IROperation node_to_irop(NodeType type) {
    switch (type) {
        case NODE_ADD:          return IR_ADD;
        case NODE_SUB:          return IR_SUB;
        case NODE_MUL:          return IR_MUL;
        case NODE_DIV:          return IR_DIV;
        case NODE_MOD:          return IR_MOD;
        case NODE_EXP:          return IR_EXP;
        case NODE_BITAND:       return IR_BITAND;
        case NODE_BITOR:        return IR_BITOR;
        case NODE_BITXOR:       return IR_BITXOR;
        case NODE_LOGAND:       return IR_LOGAND;
        case NODE_LOGOR:        return IR_LOGOR;

        // these two are unreachable, because they should always be translated 
        // x++ --> x = x + 1
        case NODE_INCREMENT:    ERR_GENERAL("unreachable");
        case NODE_DECREMENT:    ERR_GENERAL("unreachable");

        case NODE_NEG:          return IR_NEG;
        case NODE_SHL:          return IR_SHL;
        case NODE_SHR:          return IR_SHR;
        case NODE_DEREF:        return IR_DEREF;
        case NODE_ADDR:         return IR_ADDR;
        case NODE_BITNOT:       return IR_BITNOT;
        case NODE_EQ:           return IR_EQ;
        case NODE_NE:           return IR_NE;           
        case NODE_LT:           return IR_LT;
        case NODE_LE:           return IR_LE;
        case NODE_GT:           return IR_GT;
        case NODE_GE:           return IR_GE;
        
        default:                TODO("implement this irop");
    }
}

void initialise_optim_context(OptimiserContext* ctx) {
    ctx->instructions = NULL;
    ctx->temp_var_index = (size_t)0;
    ctx->label_index = (size_t)0;
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
            TODO("lower if/for/switch/return/block statements");

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

IROperand new_label(OptimiserContext* ctx) {
    IROperand op = (IROperand) {
        .type = IROP_LABEL,
        .label_id = ctx->label_index,
    };
    ctx->label_index++;
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

        case NODE_EXPR_STMT: {
            return lower_expr(ctx, node->l_value);
        }

        // binary ops
        case NODE_ADD:
        case NODE_SUB:
        case NODE_MUL:
        case NODE_DIV:
        case NODE_MOD:
        case NODE_EXP:
        case NODE_SHR:
        case NODE_SHL:
        case NODE_EQ:
        case NODE_NE:
        case NODE_LT:
        case NODE_LE:
        case NODE_GT:
        case NODE_GE:
        case NODE_BITAND:
        case NODE_BITOR:
        case NODE_BITXOR: {
            IROperand lhs = lower_expr(ctx, node->l_value);
            IROperand rhs = lower_expr(ctx, node->r_value);
            IROperand temp = new_temp(ctx);
            emit(ctx, node_to_irop(node->node_type), temp, lhs, rhs);
            return temp;
        }

        // expand increment and decrement
        case NODE_INCREMENT:
        case NODE_DECREMENT: {
            assert(node->l_value == NULL); // this should be the case if we have a unary operation, if not, check parser.c and unary ops

            IROperand rhs = (IROperand){
                .type = IROP_CONST_INT,
                .int_val = 1,
            };
            IROperand lhs = lower_expr(ctx, node->r_value);

            if (lhs.type != IROP_SYMBOL) {
                // you would get here if for instance you tried to do "5++"
                ERR_SEMANTIC(node->token, "cannot increment/decrement non-variable value");
            }

            if (node->node_type == NODE_INCREMENT) emit(ctx, IR_ADD, lhs, lhs, rhs);
            if (node->node_type == NODE_DECREMENT) emit(ctx, IR_SUB, lhs, lhs, rhs);
            return lhs;
        }

        // unary ops
        case NODE_DEREF:
        case NODE_NEG:
        case NODE_ADDR:
        case NODE_BITNOT: {
            assert(node->l_value == NULL); // this should be the case if we have a unary operation, if not, check parser.c and unary ops
            IROperand rhs = lower_expr(ctx, node->r_value);
            IROperand temp = new_temp(ctx);
            emit(ctx, node_to_irop(node->node_type), temp, IROPERAND_EMPTY, rhs);
            return temp;
        }

        // !x <==> (x == 0) i.e. (x == false)
        case NODE_NOT:
            IROperand rhs = (IROperand){
                .type = IROP_CONST_INT,
                .int_val = 0,
            };
            IROperand lhs = lower_expr(ctx, node->r_value);
            IROperand temp = new_temp(ctx);
            emit(ctx, IR_EQ, temp, lhs, rhs);
            return temp;
        
        // case (boolean)
        case NODE_LOGAND: {
            // assign a new variable (res) to 0 (false), we assume its false, because if either lhs or rhs is false, then the whole thing is false
            IROperand res = new_temp(ctx);
            IROperand label = new_label(ctx);
            IROperand falsy = (IROperand){
                .type = IROP_CONST_INT,
                .int_val = 0,
            };
            emit(ctx, IR_ASSIGN, res, falsy, IROPERAND_EMPTY);

            IROperand lhs = lower_expr(ctx, node->l_value);
            IROperand rhs = lower_expr(ctx, node->r_value);

            // if lhs == 0 or rhs == 0 skip straight to the label, because we know lhs && rhs is false
            emit(ctx, IR_JUMP_IF_ZERO, IROPERAND_EMPTY, lhs, label);
            emit(ctx, IR_JUMP_IF_ZERO, IROPERAND_EMPTY, rhs, label);

            IROperand truthy = (IROperand){
                .type = IROP_CONST_INT,
                .int_val = 1,
            };
            emit(ctx, IR_ASSIGN, res, truthy, IROPERAND_EMPTY);

            emit(ctx, IR_LABEL, label, IROPERAND_EMPTY, IROPERAND_EMPTY);
            return res;

            /*
             * basically this translates to
             *
             * res = false
             * if lhs == false or rhs == false:
             *      goto label
             * res = true
             * label:
             * return res
             *
             * so if either is false, we skip the res = true step
             */
        }

        // case (boolean)
        case NODE_LOGOR: {
            // assign a new variable (res) to 1 (true), we assume its true, because if either lhs or rhs is true, then the whole thing is true
            IROperand res = new_temp(ctx);
            IROperand label = new_label(ctx);
            IROperand truthy = (IROperand) {
                .type = IROP_CONST_INT,
                .int_val = 1,
            };
            emit(ctx, IR_ASSIGN, res, truthy, IROPERAND_EMPTY);

            IROperand lhs = lower_expr(ctx, node->l_value);
            IROperand rhs = lower_expr(ctx, node->r_value);

            // if lhs == 1 or rhs == 1 skip straight to the label, because we know lhs || rhs is true
            emit(ctx, IR_JUMP_IF_NONZERO, IROPERAND_EMPTY, lhs, label);
            emit(ctx, IR_JUMP_IF_NONZERO, IROPERAND_EMPTY, rhs, label);

            IROperand falsy = (IROperand){
                .type = IROP_CONST_INT,
                .int_val = 0,
            };
            emit(ctx, IR_ASSIGN, res, falsy, IROPERAND_EMPTY);

            emit(ctx, IR_LABEL, label, IROPERAND_EMPTY, IROPERAND_EMPTY);
            return res;

            /*
             * basically this translates to
             *
             * res = true
             * if lhs == true or rhs == true:
             *      goto label
             * res = false
             * label:
             * return res
             *
             * so if either is true, we skip the res = false step
             */
        }

        // idk yet
        case NODE_COND:
        case NODE_COMMA:
        case NODE_MEMBER:
        default: 
            printf("%d %s\n", node->node_type, node_to_str(node->node_type));
            TODO("lower specific expression type");
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
        case IR_NEG:        return "-";
        case IR_SHL:        return "<<";
        case IR_SHR:        return ">>";
        case IR_DEREF:      return "*";
        case IR_ADDR:       return "&";
        case IR_BITNOT:     return "~";
        case IR_CALL:       return "CALL";
        case IR_JUMP:       return "JUMP";
        default:            return "UNKNOWN";
    }
} 

void print_ir_operand(IROperand op) {
    switch (op.type) {
        case IROP_TEMP:         printf("$t%lu", op.temp_id); return;
        case IROP_SYMBOL:       printf("%s", op.sym->name); return;
        case IROP_CONST_INT:    printf("%lu", op.int_val); return;
        case IROP_CONST_FLOAT:  printf("%Lf", op.float_val); return;
        case IROP_LABEL:        printf("$l%zu", op.label_id); return;

        case IROP_EMPTY:
        default:                printf("");
    }
}

void print_ir(IRInstruction* instruction) {
    if (!instruction) return;
    if (instruction->op == IR_ASSIGN) {
        print_ir_operand(instruction->dest);
        printf("=");
        print_ir_operand(instruction->src1);
    } else if (instruction->op == IR_JUMP_IF_ZERO) {
        printf("JUMP TO ");
        print_ir_operand(instruction->src2);
        printf(" IF ");
        print_ir_operand(instruction->src1);
        printf(" ZERO\n");
    } else if (instruction->op == IR_JUMP_IF_NONZERO) {
        printf("JUMP TO ");
        print_ir_operand(instruction->src2);
        printf(" IF ");
        print_ir_operand(instruction->src1);
        printf(" NONZERO");
    } else if (instruction->op == IR_LABEL) {
        printf("\nLABEL "); print_ir_operand(instruction->dest); printf(":");
    } else {
        print_ir_operand(instruction->dest);
        printf("=");
        print_ir_operand(instruction->src1);
        printf("%s", irop_to_str(instruction->op));
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