#include "arena.h"

#include <stdlib.h>

typedef unsigned char byte; 

struct ArenaBlock {
    ArenaBlock* next;
    size_t capacity;
    size_t used;
    byte data[];
};

struct Arena {
    ArenaBlock* head;
    ArenaBlock* current;
    size_t block_size;
};

ArenaBlock* new_arena_block(size_t capacity) {
    ArenaBlock* block = malloc(sizeof(ArenaBlock) + capacity);
    block->next = NULL;
    block->capacity = capacity;
    block->used = (size_t)0;
    return block;
}

void arena_init(void) {
    return;
}

void* arena_alloc(Arena* arena, size_t size) {
    return NULL;
}

void arena_reset(Arena* arena) {
    return;
}

void arena_destroy(Arena* arena) {
    
}

