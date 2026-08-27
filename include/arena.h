#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdalign.h>

/*
 * Majorly inspired by https://github.com/Magicalbat/videos/blob/main/arena.c 
 */

#define KiB(n)                  ((uint64_t)(n) << 10)
#define MiB(n)                  ((uint64_t)(n) << 20)
#define GiB(n)                  ((uint64_t)(n) << 30)

// round n up to nearest multiple of p, where p is a power of 2
#define ALIGN_UP_POW2(n, p)     (((uint64_t)(n) + ((uint64_t)(p) - 1)) & (~((uint64_t)(p) - 1))) 

typedef struct Arena Arena;


#define ARENA_ALIGN             (alignof(max_align_t))
#define ARENA_BASE_POS          (sizeof(Arena))

Arena* arena_create(uint64_t reserve_size, uint64_t commit_size);
void arena_destroy(Arena* arena);
void* arena_alloc(Arena* arena, uint64_t size);
void arena_pop(Arena* arena, uint64_t size);
void arena_pop_to(Arena* arena, uint64_t pos);
void arena_clear(Arena* arena);

#define DEFAULT_RESERVE             GiB(1)
#define DEFAULT_COMMIT              MiB(1)
#define NEW_ARENA                   (arena_create(DEFAULT_RESERVE, DEFAULT_COMMIT))   

// equivalent to calloc, arena is the arena to allocate to, T is the object type, count is the amount of objects you want to allocate 
// i.e. ASTNode* node = PALLOCT(arena, ASTNode, 1);
#define PALLOCT(arena, T, count)    ((T*)arena_alloc((arena), (sizeof(T) * (count))))

// raw version, specify an arena and a specific size you want to allocate
// i.e. ASTNode* node = PALLOCS(arena, sizeof(ASTNode));
#define PALLOCS(arena, size)        (arena_alloc((arena), (size)))

#endif