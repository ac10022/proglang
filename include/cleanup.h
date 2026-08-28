#ifndef CLEANUP_H
#define CLEANUP_H

#include <stdbool.h>
#include "arena.h"

typedef struct Notice Notice;

typedef enum {
    NOTICE_ERROR,
    NOTICE_WARNING,
    NOTICE_INFO,
} NoticeType;

struct Notice {
    NoticeType type;
    char* message;
    bool requires_halt;
    Notice* next;
};

typedef struct {
    Notice* notices;
    Arena* arena;
} CleanupContext;

CleanupContext* new_cleanup_context(Arena* arena);
bool has_error_notice(CleanupContext* cl_ctx);
void append_new_notice(CleanupContext* cl_ctx, NoticeType type, bool requires_halt, const char* msg, ...);
void compilation_exit(CleanupContext* cl_ctx, bool preemptive);

#endif