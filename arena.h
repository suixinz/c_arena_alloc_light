/**
 * @file arena.h / arena.c
 * @brief Generic linked-list multi-block arena allocator
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

#pragma once

#include <stdlib.h>
#include <stdbool.h>

/* ========== Configuration Macros ========== */

#ifndef ALIGNMENT 
/**
 * @brief Memory alignment in bytes. Must be a power of two.
 * Default is 8 bytes, sufficient for most types (pointers, double, etc.).
 */
#define ALIGNMENT 8
#endif

#ifndef ARENA_BLOCK_DEFAULT_CAPACITY
/**
 * @brief Default initial capacity of a single block in bytes.
 * Default is 1MB. Capacity doubles on each growth if more space is needed.
 */
#define ARENA_BLOCK_DEFAULT_CAPACITY (1024 * 1024)
#endif

/* ========== Type Definitions ========== */

/**
 * @brief Opaque arena type.
 *
 * Callers hold an arena* and never access internal fields directly.
 * Internally manages a linked list of memory blocks.
 */

typedef struct arena arena;

/* ========== API ========== */

/**
 * @brief Creates a new arena instance.
 * @return Pointer to arena on success, NULL on failure.
 *
 * Note: Does not allocate any memory blocks immediately.
 * The first block is lazily created on the first allocation call.
 */
arena* arena_create();

/**
 * @brief Allocates and zero-initializes memory from the arena (analogous to calloc).
 * @param arena_pool    Arena instance pointer.
 * @param count Number of elements.
 * @param size  Size of each element in bytes.
 * @return Aligned pointer to zeroed memory on success, NULL on failure.
 *
 * Alignment guarantee: returned pointer address is a multiple of ALIGNMENT.
 * Overflow check: returns NULL if count * size overflows.
 * Returns NULL if arena is NULL.
 */
void* arena_calloc(arena* arena_pool, size_t count, size_t size);

/**
 * @brief Allocates memory from the arena without zero-initialization (analogous to malloc).
 * @param arena_pool Arena instance pointer.
 * @param size Number of bytes to allocate.
 * @return Aligned pointer to uninitialized memory on success, NULL on failure.
 *
 * Alignment guarantee: returned pointer address is a multiple of ALIGNMENT.
 * Returns NULL if ar is NULL.
 */
void* arena_malloc(arena* arena_pool, size_t size);

/**
 * @brief Destroys the arena instance, freeing all internal blocks and the arena itself.
 * @param arena_pool Arena instance pointer. May be NULL (safe no-op).
 *
 * After this call, the arena_pool pointer is invalid and must not be used.
 * Passing NULL is safe and does nothing.
 * A new arena can be created afterward via arena_create().
 */
void arena_destroy(arena* arena_pool);