#ifndef ASTNODE_H
#define ASTNODE_H

typedef enum {
  NODE_NULL_EXPR, // Do nothing
  NODE_ADD,       // +
  NODE_SUB,       // -
  NODE_MUL,       // *
  NODE_DIV,       // /
  NODE_NEG,       // unary -
  NODE_MOD,       // %
  NODE_BITAND,    // &
  NODE_BITOR,     // |
  NODE_BITXOR,    // ^
  NODE_SHL,       // <<
  NODE_SHR,       // >>
  NODE_EQ,        // ==
  NODE_NE,        // !=
  NODE_LT,        // <
  NODE_LE,        // <=
  NODE_ASSIGN,    // =
  NODE_COND,      // ?:
  NODE_COMMA,     // ,
  NODE_MEMBER,    // . (struct member access)
  NODE_ADDR,      // unary &
  NODE_DEREF,     // unary *
  NODE_NOT,       // !
  NODE_BITNOT,    // ~
  NODE_LOGAND,    // &&
  NODE_LOGOR,     // ||
  NODE_RETURN,    // "return"
  NODE_IF,        // "if"
  NODE_FOR,       // "for" or "while"
  NODE_BLOCK,     // { ... }
  NODE_LABEL,     // Labeled statement
  NODE_LABEL_VAL, // [GNU] Labels-as-values
  NODE_FUNCALL,   // Function call
  NODE_EXPR_STMT, // Expression statement
  NODE_STMT_EXPR, // Statement expression
  NODE_VAR,       // Variable
  NODE_VLA_PTR,   // VLA designator
  NODE_NUM,       // Integer
  NODE_CAST,      // Type cast
} NodeType;

typedef struct ASTNode {
    NodeType type;
    union {
        struct {
            char op;
            struct ASTNode *left;
            struct ASTNode *right;
        } bin_op;
        int int_value;
        struct {
            char *var_name;
            struct ASTNode *value;
        } assign;
    } data;
} ASTNode;

#endif

