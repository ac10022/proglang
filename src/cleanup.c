#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "cleanup.h"

/*
 * This module exists to free used memory once compilation is complete, and also handles errors.
 * We should not stop exactly once we hit the first error, rather we should continue trying to compile until we reach an error where we cannot infer and are forced to stop compilation.
 */

CleanupContext* new_cleanup_context(void) {
    CleanupContext* cl_ctx = calloc(1, sizeof(CleanupContext));
    cl_ctx->notices = NULL;
    return cl_ctx;
}

void destroy_cleanup_context(CleanupContext* cl_ctx) {
    if (!cl_ctx) return;

    Notice* current = cl_ctx->notices;
    while (current != NULL) {
        Notice* next = current->next;
        free(current->message);
        free(current);
        current = next;
    }

    free(cl_ctx);
}

bool has_error_notice(CleanupContext* cl_ctx) {
    if (cl_ctx->notices != NULL) {
        Notice *end = cl_ctx->notices;
		for (; end != NULL; end = end->next) {
            if (end->type == NOTICE_ERROR) return true;
        }
    }
    return false;
}

void append_new_notice(CleanupContext* cl_ctx, NoticeType type, bool requires_halt, const char* msg, ...) {
    if (!cl_ctx) return;
    if (!msg) return;

    va_list args, args_copy;
    va_start(args, msg);
    va_copy(args_copy, args);

    // find the required string length
    int len = vsnprintf(NULL, 0, msg, args);
    va_end(args);

    // formatting failure
    if (len < 0) {
        va_end(args_copy);
        return;
    }

    char* fmt_msg = malloc(len + 1);

    vsnprintf(fmt_msg, len + 1, msg, args_copy);
    va_end(args_copy);

    Notice* notice = calloc(1, sizeof(Notice));
    notice->next = NULL;
    notice->type = type;
    notice->requires_halt = requires_halt; 
    notice->message = fmt_msg;

    if (cl_ctx->notices == NULL) cl_ctx->notices = notice;
    else {
        Notice *end = cl_ctx->notices;
		for (; end->next != NULL; end = end->next);
		end->next = notice;
    }

    // we must stop compilation
    if (requires_halt) compilation_exit(cl_ctx, true);
}

void print_all_notices(CleanupContext* cl_ctx) {
    if (cl_ctx->notices != NULL) {
        Notice *end = cl_ctx->notices;
		for (; end != NULL; end = end->next) printf("%s", end->message);
    }
}

void cleanup(CleanupContext* cl_ctx) {
    // not sure exactly yet how this should work but
    // where possible during the program, we should free once we are done, e.g., once the parser has finished, we can free everything we have used temporarily except for the AST
    // at the end we are going to have a whole system of IRInstructions, ASTNodes, Tokens, TypeInfos etc which will need to be freed, this is what this function should be for
    return;
} 

void compilation_exit(CleanupContext* cl_ctx, bool preemptive) {
    // print all notices, then cleanup, then exit
    print_all_notices(cl_ctx);
    cleanup(cl_ctx);

    if (preemptive) {
        fprintf(stderr, "Compilation FAILED ...\n");
        exit(1);
    }

    else {
        fprintf(stdout, "Compilation SUCCESS ...\n");
        exit(0);
    }
}


