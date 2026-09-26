// to expose common linux features, like madvise
#ifdef __linux__
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif // _DEFAULT_SOURCE
#endif // __linux__

#include "arena.h"
#include <string.h>
#include <assert.h>

// Allocator is currently NOT thread-safe

struct Arena {
    uint64_t reserve_size;
    uint64_t commit_size;
    uint64_t pos;
    uint64_t commit_pos;
};

typedef unsigned char byte;
#define ARENA_BASE_POS          (sizeof(Arena))

#ifdef __linux__

#include <unistd.h>
#include <sys/mman.h>

uint32_t plat_get_pagesize(void) {
    return (uint32_t)sysconf(_SC_PAGE_SIZE);
}

/*
 * 'mmap' is a low level virtual memory call, this gives us advantages over typical malloc
 * We reserve a large block in advance, whereas standard memory calls has to search heap data structures every call.
 */

/*
 * Reserve a contiguous block of virtual address space.
 */
void* mem_reserve(uint64_t size) {
    void* out = mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (out == MAP_FAILED) return NULL;
    return out;
}

/*
 * Allow virtual addresses to be used in RAM.
 */
bool mem_commit(void* ptr, uint64_t size) {
    return mprotect(ptr, size, PROT_READ | PROT_WRITE) == 0;
}

/*
 * RAM given back to OS, do not unreserve virtual memory space.
 */
bool mem_decommit(void* ptr, uint64_t size) {
    int ret = mprotect(ptr, size, PROT_NONE);
    if (ret != 0) return false;
    return madvise(ptr, size, MADV_DONTNEED) == 0; 
}

/*
 * Unreserve virtual memory space 
 */
bool mem_release(void* ptr, uint64_t size) {
    return munmap(ptr, size) == 0;
}

#endif

// TODO: else if WINDOWS :(

Arena* arena_create(uint64_t reserve_size, uint64_t commit_size) {
    assert(commit_size != 0);
    assert(commit_size <= reserve_size); 

    uint32_t page_size = plat_get_pagesize();

    reserve_size = ALIGN_UP_POW2(reserve_size, page_size);
    commit_size = ALIGN_UP_POW2(commit_size, page_size);

    // reserve virtual address space
    Arena* arena = mem_reserve(reserve_size); 
    
    if (!arena) return NULL;

    // first chunk with physical ram, so that we can populate arena fields
    if (!mem_commit(arena, commit_size)) {
        mem_release(arena, reserve_size);
        return NULL;
    }

    // populate arena fields
    arena->reserve_size = reserve_size;
    arena->commit_size = commit_size;
    arena->commit_pos = commit_size;
    arena->pos = ARENA_BASE_POS;

    return arena;
}

void arena_destroy(Arena* arena) {
    mem_release(arena, arena->reserve_size);
}

void* arena_alloc(Arena* arena, uint64_t size) {
    uint64_t pos_aligned = ALIGN_UP_POW2(arena->pos, ARENA_ALIGN);
    uint64_t new_pos = pos_aligned + size;

    // memory allocation failure
    if (pos_aligned > arena->reserve_size || size > arena->reserve_size - pos_aligned) return NULL;

    // check if this allocation will exceed the amount of physical ram held
    // if so we request more ram
    if (new_pos > arena->commit_pos) {
        // round up to the next commit size boundary
        uint64_t new_commit_pos = new_pos;
        new_commit_pos += (arena->commit_size - 1);
        new_commit_pos -= (new_commit_pos % arena->commit_size);
        new_commit_pos = (new_commit_pos < arena->reserve_size) ? new_commit_pos : arena->reserve_size;

        byte* mem = (byte*)arena + arena->commit_pos;
        uint64_t commit_size = new_commit_pos - arena->commit_pos;

        if (!mem_commit(mem, commit_size)) return NULL;
        arena->commit_pos = new_commit_pos;
    }

    arena->pos = new_pos;
    byte* out = (byte*)arena + pos_aligned;

    // we are assuming all alloc calls are zero'ed, in the compiler most allocations require this so we just set it as default
    memset(out, 0, size);
    return out;
}

void arena_pop(Arena* arena, uint64_t size) {
    uint64_t max_size = arena->pos - ARENA_BASE_POS;
    size = (size < max_size) ? size : max_size;
    arena->pos -= size;
}

void arena_pop_to(Arena* arena, uint64_t pos) {
    uint64_t size = pos < arena->pos ? arena->pos - pos : 0;
    arena_pop(arena, size);
}

void arena_clear(Arena* arena) {
    arena_pop_to(arena, ARENA_BASE_POS);
}