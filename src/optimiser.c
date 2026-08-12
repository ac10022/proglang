#include "optimiser.h"
#include "base.h"

/*
 * TODO:
 *  * lower functions, return statements, struct members etc.
 *  * optimiser (see below)
 *  * work out how the fuck scope works in asm
 */

/*
 * Main entry point of the OPTIMISER component; accepts an AST from the parser (the head) and returns a linked list of IR instructions.
 */
IRInstruction* ast_to_ir(ASTNode* root, CompilerContext* c_ctx) {
    if (!root) ERR_GENERAL("Invalid AST provided for IR parsing");

    OptimiserContext o_ctx = {};
    initialise_optim_context(&o_ctx);

    // stage 1: lowering
    for (ASTNode* statement = root; statement != NULL; statement = statement->next) {
        lower(&o_ctx, statement);
    }
    emit(&o_ctx, IR_HALT, IROPERAND_EMPTY, IROPERAND_EMPTY, IROPERAND_EMPTY);

    // stage 2: optimising
    if (c_ctx->flags & CF_USE_OPTIMISATIONS) {
        optimise(&o_ctx);
    }

    return o_ctx.instructions;
}

/*
 * Helper function to convert between NodeType and IROperation
 */
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

    return (IROperation){0}; // here to shut up compiler
}

/*
 * Initialise all fields of an OptimiserContext object to default values once it has been allocated on the stack.
 */
void initialise_optim_context(OptimiserContext* ctx) {
    ctx->instructions = NULL;
    ctx->temp_var_index = (size_t)0;
    ctx->label_index = (size_t)0;
}

/*
 * Pushes an instruction onto the end of the context's instruction list.
 */
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

/*
 * Create an IR instruction and push it to the end of the context's instruction list.
 * The instruction will be of the form: d = [src1] <op> [src2]
 */
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

/*
 * the first stage is to convert ast -> ir, this process is called lowering
 * what is happening here, is we are just traversing the tree and calling lower on every node, then lowering subnodes in recursive descent
 * every time there is an instruction which we should keep, then we emit it (emit()) to the ir linked list of instructions
 * 
 * for now, we do not do any optimisation while we finish the lowering process
 */

/*
 * Recursively lower the AST into an IR instruction linked list (held in the OptimiserContext object)
 * This function accounts for statements, e.g., 'for', 'while', if instead we have a expression, we fall through to lower_expr.
 */
void lower(OptimiserContext* ctx, ASTNode* node) {
    if (!ctx) return;
    if (!node) return;

    switch (node->node_type) {
        case NODE_VARAIBLE_DECLARATION: {
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
        }
            
        case NODE_IF: {
            /*
             *  if (condition) {
             *      ...
             *  } else {
             *      ...
             *  }
             * 
             * translating to
             *
             *  if condition nonzero:
             *      goto success
             *  
             *  [COND FAIL STMTS]
             *  goto end
             * 
             *  success:
             *  [COND SUCCESS STMTS]
             * 
             *  end:
             */

            IROperand success = new_label(ctx);
            IROperand end = new_label(ctx);
            IROperand cond = lower_expr(ctx, node->condition);

            emit(ctx, IR_JUMP_IF_NONZERO, IROPERAND_EMPTY, cond, success);  // if condition nonzero: goto success
            lower(ctx, node->on_condition_failure);                         // [COND FAIL STMTS]
            emit(ctx, IR_JUMP, IROPERAND_EMPTY, IROPERAND_EMPTY, end);      // goto end
            emit(ctx, IR_LABEL, success, IROPERAND_EMPTY, IROPERAND_EMPTY); // success:
            lower(ctx, node->on_condition_success);                         // [COND SUCCESS STMTS]
            emit(ctx, IR_LABEL, end, IROPERAND_EMPTY, IROPERAND_EMPTY);     // end:
            break;
        }
            
        case NODE_BLOCK: {
            emit(ctx, IR_BEGIN_SCOPE, IROPERAND_EMPTY, IROPERAND_EMPTY, IROPERAND_EMPTY);
            for (ASTNode* end = node->body; end != NULL; end = end->next) lower(ctx, end);
            emit(ctx, IR_END_SCOPE, IROPERAND_EMPTY, IROPERAND_EMPTY, IROPERAND_EMPTY);
            break;
        }
            
        case NODE_FOR: {
            /*
             * for (init; cond; incr) {
             *      ...
             * }
             * 
             * translating to:
             *
             * [INIT]
             * loop:
             * if cond nonzero:     // stay in loop
             *      goto continue
             * 
             * goto exit
             * 
             * continue:
             *      [FOR STATEMENTS]
             *      [INCR]
             *      goto loop
             * 
             * exit:
             */

            IROperand loop = new_label(ctx);
            IROperand cont = new_label(ctx);
            IROperand exit = new_label(ctx);
            IROperand cond = IROPERAND_EMPTY;
            
            lower(ctx, node->initial);                                      // [INIT]
            emit(ctx, IR_LABEL, loop, IROPERAND_EMPTY, IROPERAND_EMPTY);    // loop:
            
            // infinite loop of form for (... ; ; ...) i.e. infinite loop or relying on break
            if (node->condition == NULL) {
                emit(ctx, IR_JUMP, IROPERAND_EMPTY, IROPERAND_EMPTY, cont);
            }
            else {
                cond = lower_expr(ctx, node->condition);
                emit(ctx, IR_JUMP_IF_NONZERO, IROPERAND_EMPTY, cond, cont); // if cond nonzero: goto continue
            }
            emit(ctx, IR_JUMP, IROPERAND_EMPTY, IROPERAND_EMPTY, exit);     // goto exit
            emit(ctx, IR_LABEL, cont, IROPERAND_EMPTY, IROPERAND_EMPTY);    // continue:
            lower(ctx, node->body);                                         // [FOR STATEMENTS]
            lower(ctx, node->increment);                                    // [INCR]
            emit(ctx, IR_JUMP, IROPERAND_EMPTY, IROPERAND_EMPTY, loop);     // goto loop
            emit(ctx, IR_LABEL, exit, IROPERAND_EMPTY, IROPERAND_EMPTY);    // exit:
            break;
        }

        case NODE_FUNCTION: {

            /*
             * fn foo(T1 arg1, T2 arg2 ... ) -> T { ... }
             * 
             * translating to:
             * FUNC_BEGIN 'foo'
             * [PARAMS]
             * [BODY]
             * RETURN
             * FUNC_END 'foo'
             *
             */

            IROperand fn = (IROperand) {
                .type = IROP_FUNC,
                .func_name = node->function_name,
            };

            emit(ctx, IR_BEGIN_FUNC, fn, IROPERAND_EMPTY, IROPERAND_EMPTY);

            for (ASTNode* end = node->l_value; end != NULL; end = end->next) {
                IROperand param = (IROperand) {
                    .type = IROP_SYMBOL,
                    .sym = end->variable_symbol,
                };

                emit(ctx, IR_PARAM, param, IROPERAND_EMPTY, IROPERAND_EMPTY);
            }

            lower(ctx, node->body);

            emit(ctx, IR_RETURN, IROPERAND_EMPTY, IROPERAND_EMPTY, IROPERAND_EMPTY);
            emit(ctx, IR_END_FUNC, fn, IROPERAND_EMPTY, IROPERAND_EMPTY);

            break;
        }
            
        case NODE_RETURN: {
            // return has value
            if (node->l_value) {
                IROperand val = lower_expr(ctx, node->l_value);
                emit(ctx, IR_RETURN, IROPERAND_EMPTY, val, IROPERAND_EMPTY);
            }

            // return has no value
            else {
                emit(ctx, IR_RETURN, IROPERAND_EMPTY, IROPERAND_EMPTY, IROPERAND_EMPTY);
            }

            break;
        }

        case NODE_SWITCH:
        case NODE_FUNCTION_CALL:
        case NODE_FUNCTION_DECLARATION:
            TODO("lower switch/function calls");

        default:
            lower_expr(ctx, node);
            break;
    }
}

/*
 * Create a new IR temporary variable (in DEBUG, a temp variable has the notation $t).
 */
IROperand new_temp(OptimiserContext* ctx) {
    IROperand op = (IROperand) {
        .type = IROP_TEMP,
        .temp_id = ctx->temp_var_index,
    };
    ctx->temp_var_index++;
    return op;
}

/*
 * Create a new IR label, which we can jump to using IR_JUMP or any alternatives (in DEBUG, a label has the notation $l).
 */
IROperand new_label(OptimiserContext* ctx) {
    IROperand op = (IROperand) {
        .type = IROP_LABEL,
        .label_id = ctx->label_index,
    };
    ctx->label_index++;
    return op;
}

/*
 * Recursively lowers any expression type node and emits into the IR instruction linked list (held in the OptimiserContext object).
 */
IROperand lower_expr(OptimiserContext* ctx, ASTNode* node) {
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
        case NODE_NOT: {
            IROperand lhs = lower_expr(ctx, node->r_value);
            IROperand temp = new_temp(ctx);
            emit(ctx, IR_EQ, temp, lhs, FALSE);
            return temp;
        }

        // case (boolean)
        case NODE_LOGAND: {
            // assign a new variable (res) to 0 (false), we assume its false, because if either lhs or rhs is false, then the whole thing is false
            IROperand res = new_temp(ctx);
            IROperand label = new_label(ctx);
            emit(ctx, IR_ASSIGN, res, FALSE, IROPERAND_EMPTY);

            // if lhs == 0 or rhs == 0 skip straight to the label, because we know lhs && rhs is false
            IROperand lhs = lower_expr(ctx, node->l_value);
            emit(ctx, IR_JUMP_IF_ZERO, IROPERAND_EMPTY, lhs, label);
            
            IROperand rhs = lower_expr(ctx, node->r_value);
            emit(ctx, IR_JUMP_IF_ZERO, IROPERAND_EMPTY, rhs, label);

            emit(ctx, IR_ASSIGN, res, TRUE, IROPERAND_EMPTY);

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
            emit(ctx, IR_ASSIGN, res, TRUE, IROPERAND_EMPTY);

            // if lhs == 1 or rhs == 1 skip straight to the label, because we know lhs || rhs is true
            IROperand lhs = lower_expr(ctx, node->l_value);
            emit(ctx, IR_JUMP_IF_NONZERO, IROPERAND_EMPTY, lhs, label);
            
            IROperand rhs = lower_expr(ctx, node->r_value);
            emit(ctx, IR_JUMP_IF_NONZERO, IROPERAND_EMPTY, rhs, label);

            emit(ctx, IR_ASSIGN, res, FALSE, IROPERAND_EMPTY);

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
#ifdef DEBUG
            printf("%d %s\n", node->node_type, node_to_str(node->node_type));
#endif
            TODO("lower specific expression type");
    }

    return IROPERAND_EMPTY; // here to shut up compiler
}

/*
 * the second stage of the optimiser is the actual optimisation, so we go through the IR and pattern match on typical inefficient patterns
 *
 * what we should look for here: 
 *  
 *  *   loop invariant code, where we have statements in loops which may not change after a set amount of loops, e.g.,
 *          b8 bool = false
 *          for (u32 i in 1..10) {
 *              bool = true // <--- does not need to be in the loop
 *          }
 * 
 *  *   constants which can be evaluated directly e.g,
 *          u32 t = 60 * 60 * 24
 *      
 *  *   strength reduction, replacing expensive operations with cheaper ones e.g.,
 *          u32 i = 2 * 16   <===>   u32 i = 2 << 4
 *  
 *  *   dead code elimination, unused temp varaibles, labels which are not jumped to, statements which do not change the state of the program, e.g.,
 *          u32 i = 10;
 *          i = 10; // <--- redundant 
 * 
 *  *   common expressions, expressions which have already been evaluated and can be reused, e.g.,
 *          u32 t = 60 * 60 * 24
 *          u32 t2 = 60 * 60 * 24   <===>   u32 t2 = t 
 */

/*
 * Loop through the context's IR linked list, and greedily look for optimisations.  
 */
void optimise(OptimiserContext* ctx) {
    return;
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
        case IR_LE:         return "<=";
        case IR_LT:         return "<";
        case IR_EQ:         return "==";
        case IR_GT:         return ">";
        case IR_GE:         return ">=";
        case IR_NE:         return "!=";
        case IR_CALL:       return "CALL";
        case IR_JUMP:       return "JUMP";
        case IR_PARAM:      return "PARAM";
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
        case IROP_FUNC:         printf("%s", op.func_name); return;

        case IROP_EMPTY:
        default:                return;
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
        printf(" NONZERO\n");
    } else if (instruction->op == IR_JUMP) {
        printf("JUMP TO ");
        print_ir_operand(instruction->src2);
    } else if (instruction->op == IR_LABEL) {
        printf("\nLABEL "); print_ir_operand(instruction->dest); printf(":");
    } else if (instruction->op == IR_BEGIN_SCOPE) {
        printf("* BEGIN NEW SCOPE *");
    } else if (instruction->op == IR_END_SCOPE) {
        printf("* END CURRENT SCOPE *");
    } else if (instruction->op == IR_HALT) {
        printf("* PROGRAM HALT *");
    } else if (instruction->op == IR_BEGIN_FUNC) {
        printf("BEGIN FUNCTION ");
        print_ir_operand(instruction->dest);
    } else if (instruction->op == IR_END_FUNC) {
        printf("END FUNCTION ");
        print_ir_operand(instruction->dest);
    } else if (instruction->op == IR_RETURN) {
        printf("RETURN ");
        print_ir_operand(instruction->src1);
    } else if (instruction->op == IR_PARAM) {
        printf("PARAM ");
        print_ir_operand(instruction->dest);
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