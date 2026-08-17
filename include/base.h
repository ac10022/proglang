#ifndef BASE_H
#define BASE_H

#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "cleanup.h"

/*
 * DATA TYPES
 */

typedef struct {
	uint64_t flags;
	char *filepath;
	char *outpath;

    CleanupContext* cl_ctx;
} CompilerContext;

enum {
	CF_GENERATE_ASSEMBLY 	= (1ULL << 0),
	CF_USE_OPTIMISATIONS 	= (1ULL << 1),

    // DEBUG only command flags
    CF_AST_TRACE            = (1ULL << 62),
    CF_IR_TRACE             = (1ULL << 63),
};

/*
 * CONSTANTS
 */

#define MAX_BUFFER_LENGTH 4096

/*
 * MACROS
 */

/*
 * Obsolete old error macros 
 */

#define STOP_COMPILATION() \
    do { \
        fprintf(stderr, "Stopping compilation ...\n"); \
        exit(1); \
    } while (0) 

#define ERR_GENERAL(fmt, ...) \
    do { \
        fprintf(stderr, "ERROR:\t" fmt "\n", ##__VA_ARGS__); \
        STOP_COMPILATION(); \
    } while (0)

#define ERR_SYNTAX(tok_deref, expected) ERR_GENERAL("%s:%lu:\tSyntax error; expected "expected", got '%.*s'.", (tok_deref)->source->filepath, (tok_deref)->line_number, TOK_STR_VAL(tok_deref)); 

#define ERR_SEMANTIC(tok_deref, msg) ERR_GENERAL("%s:%lu:\tSemantic error; "msg", got '%.*s'.", (tok_deref)->source->filepath, (tok_deref)->line_number, TOK_STR_VAL(tok_deref));

/*
 * New cleanup error macros
 */

#define ERR_GENERAL_CTX(cl_ctx, fmt, ...) \
    do { \
        append_new_notice((cl_ctx), NOTICE_ERROR, false, "ERROR:\t" fmt "\n", ##__VA_ARGS__); \
    } while (0)

#define ERR_HALT_CTX(cl_ctx, fmt, ...) \
    do { \
        append_new_notice((cl_ctx), NOTICE_ERROR, true, "ERROR:\t" fmt "\n", ##__VA_ARGS__); \
    } while (0)

#define ERR_SYNTAX_CTX(cl_ctx, tok_deref, expected, exit) \
    do { \
        append_new_notice((cl_ctx), NOTICE_ERROR, (exit), "%s:%lu:\tSyntax error; expected " expected ", got '%.*s'.\n", \
            (tok_deref)->source->filepath, (tok_deref)->line_number, TOK_STR_VAL(tok_deref)); \
    } while (0)

#define ERR_SEMANTIC_CTX(cl_ctx, tok_deref, msg, exit) \
    do { \
        append_new_notice((cl_ctx), NOTICE_ERROR, (exit), "%s:%lu:\tSemantic error; " msg ", got '%.*s'.\n", \
            (tok_deref)->source->filepath, (tok_deref)->line_number, TOK_STR_VAL(tok_deref)); \
    } while (0)

#define WARN_CTX(cl_ctx, fmt, ...) \
    do { \
        append_new_notice((cl_ctx), NOTICE_WARNING, false, "WARNING:\t" fmt "\n", ##__VA_ARGS__); \
    } while (0)

#define DEBUG_TOKEN_STR(tok) printf("%.*s\n", TOK_STR_VAL(tok))

#define WARN_GENERAL(fmt, ...) fprintf(stderr, "WARNING:\t" fmt "\n", ##__VA_ARGS__)

#define STR_STARTS_WITH(str, start) (strncmp(str, start, strlen(start)) == 0)

#define CHR_IS_NEWLINE(chr) ((chr) == '\n' || (chr) == '\r')

#define TOK_STR_VAL(tok) (int)((tok)->length), ((tok)->location)

#define TODO(msg) assert(0 && (msg))

#endif

