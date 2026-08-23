/**
 * @file arena.c
 * @brief Arena allocator implementation
 *
 * Copyright (c) 2026 [Zhang Shuwen / suixinz]
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "arena.h"
#include <string.h>

typedef struct block{

    unsigned char* mem;  /**< Pointer to the actual memory buffer. */
    size_t offset;       /**< Current used offset in bytes. */
    size_t capacity;     /**< Total capacity of this block in bytes. */

    struct block* next;  /**< Pointer to the next block; NULL at list tail. */

}block;

/** @brief Arena instance managing a linked list of blocks. */
struct arena{

    block* head;        /**< List head, used for traversal during destroy. */
    block* current;     /**< Currently active block for new allocations. */

};


/* ========== Internal Utilities ========== */

/**
 * @brief Rounds n up to the nearest multiple of ALIGNMENT.
 * @param n The original size.
 * @return The aligned size.
 *
 * ALIGNMENT must be a power of two.
 * Algorithm: (n + ALIGNMENT - 1) & ~(ALIGNMENT - 1)
 */
static inline size_t align_up(size_t n){

    return ( (n + ALIGNMENT - 1) & (~(ALIGNMENT - 1)) );

}

/* ========== API Implementation ========== */

arena* arena_create(){

    arena* arena_struct = (arena*)calloc(1, sizeof(arena));

    if(arena_struct == NULL){
        return NULL;
    }
    /* head and current are zero-initialized to NULL by calloc.
     * The first block is lazily allocated on first use. */

    return arena_struct;

}

/**
 * @brief Appends a new block to the end of the arena's block list.
 * @param arena_pool Arena instance.
 * @param n  Requested allocation size (ensures the new block can hold at least n bytes).
 * @return true on success, false on failure.
 */
static bool arena_block_insert_tail(arena* arena_pool, size_t n){

    /* Allocate block descriptor */
    block* new_block = (block*)malloc(sizeof(block));

    if(new_block == NULL){
        return false;
    }

    /* Determine capacity: start from default, double until >= n */
    size_t new_block_capacity = ARENA_BLOCK_DEFAULT_CAPACITY;

    while (new_block_capacity < n){
        new_block_capacity <<= 1;
    }
    
    /* Allocate the actual memory buffer */
    new_block->mem = (unsigned char*)malloc(new_block_capacity * sizeof(unsigned char));

    if(new_block->mem == NULL){
        free(new_block);
        return false;
    }

    new_block->offset = 0;
    new_block->capacity = new_block_capacity;
    new_block->next = NULL;

    /* Attach to list tail */
    if(arena_pool->head == NULL && arena_pool->current == NULL){
        arena_pool->head = arena_pool->current = new_block;
    }
    else{
        arena_pool->current->next = new_block;
        arena_pool->current = new_block;
    }

    return true;

}

/**
 * @brief Checks whether the current block has enough remaining space.
 * @param arena_pool Arena instance.
 * @param n  Requested allocation size in bytes.
 * @return true if space is insufficient (new block needed), false if space is adequate.
 */
static bool out_of_current_block_capacity(arena* arena_pool, size_t n){

    if(arena_pool->current == NULL){
        return true;
    }/* No block exists yet */

    size_t padded_offset = align_up(arena_pool->current->offset);

    if(padded_offset + n > arena_pool->current->capacity){
        return true;
    }

    return false;

}

void* arena_calloc(arena* arena_pool, size_t count, size_t size){
    
    if(count == 0 || size == 0){
        return NULL;
    }

    if (count != 0 && size > SIZE_MAX / count){
        return NULL;
    }/* Multiplication overflow */

    size_t n_uchar = count * size;

    /* Validate inputs */
    if(arena_pool == NULL){
        return NULL;
    }

    /* Ensure current block has room; allocate a new one if needed */
    if(out_of_current_block_capacity(arena_pool, n_uchar)){

        if(arena_block_insert_tail(arena_pool, n_uchar) == false){
            return NULL;
        }

    }

    /* Align the start address */
    size_t padded_offset = align_up(arena_pool->current->offset);

    void* ptr = arena_pool->current->mem + padded_offset;

    /* Zero-initialize the allocated region */
    memset(ptr, 0, n_uchar);

    /* Advance offset */
    arena_pool->current->offset = padded_offset + n_uchar;

    return ptr;

}

void* arena_malloc(arena* arena_pool, size_t size){

    if (size == 0){
        return NULL;
    } 

     /* Validate inputs */
    if(arena_pool == NULL){
        return NULL;
    }

    /* Ensure current block has room */
    if(out_of_current_block_capacity(arena_pool, size)){

        if(arena_block_insert_tail(arena_pool, size) == false){
            return NULL;
        }

    }

    /* Align the start address */
    size_t padded_offset = align_up(arena_pool->current->offset);

    void* ptr = arena_pool->current->mem + padded_offset;

    /* Advance offset (no zero-initialization) */
    arena_pool->current->offset = padded_offset + size;

    return ptr;

}

/**
 * @brief Frees a single block and its associated memory buffer.
 * @param bck Block pointer; safe to pass NULL.
 */
static void block_destroy(block* bck){

    if(bck == NULL){
        return;
    }

    free(bck->mem);
    free(bck);

}

void arena_destroy(arena* arena_pool){

    if(arena_pool == NULL){
        return;
    }

    if(arena_pool->head == NULL){
        return;
    }

    /* Traverse and free all blocks */
    while(arena_pool->head != NULL){

        block* cur_block = arena_pool->head;
        arena_pool->head = cur_block->next;

        block_destroy(cur_block);

    }

    /* Free the arena struct itself */
    free(arena_pool);

}