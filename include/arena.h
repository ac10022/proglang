#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

typedef struct ArenaBlock ArenaBlock;
typedef struct Arena Arena;

void arena_init(void);
void* arena_alloc(Arena* arena, size_t size);
void arena_reset(Arena* arena);
void arena_destroy(Arena* arena);

#define CALLOC(a, size) arena_alloc((a), (size))
#define CALLOC_C(a, size, count) CALLOC(a, (size * count))

#endif