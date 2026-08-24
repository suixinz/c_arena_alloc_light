# Arena Allocator

A lightweight, general-purpose **arena (region) allocator** written in C, featuring a linked-list multi-block design with zero individual `free` calls during use.

## Features

- **Multi-block linked list**: allocations never move; old pointers stay valid
- **8-byte alignment**: safe for all common types (`double`, pointers, SIMD)
- **Two allocation APIs**:
  - `arena_malloc` — fast pointer-bump, no zero-initialization
  - `arena_calloc` — allocation + zero-init (analogous to `calloc`)
- **Overflow protection**: `count * size` checked before allocation
- **Multi-instance**: no global state, each instance independently managed
- **Lazy block creation**: first block allocated on first use
- **Zero `free` during use**: all memory released in one `arena_destroy()` call
- **Clean C API**: opaque type, no internal details exposed in header

## Build

bash
GCC / Clang / MinGW-w64
gcc -std=c11 -Wall -O2 arena.c test_arena.c -o test_arena
./test_arena
With benchmark
gcc -std=c11 -Wall -O2 arena.c benchmark.c -o benchmark
./benchmark

## Usage

c
include "arena.h"
int main() {
/* Create arena */
arena* ar = arena_create();
if (!ar) return 1;

/* Allocate */
int* nums = arena_malloc(ar, 10 * sizeof(int));
char* str = arena_calloc(ar, 64, sizeof(char));

/* Use normally */
nums[0] = 42;
strcpy(str, "hello arena");

/* One call to free everything */
arena_destroy(ar);
return 0;
}

### Multiple Instances

c
arena* ast_arena = arena_create();
arena* tmp_arena = arena_create();
/* ... use independently ... */
arena_destroy(ast_arena);
arena_destroy(tmp_arena);

## API

| Function | Description |
|----------|-------------|
| `arena_create()` | Create a new arena instance |
| `arena_malloc(arena_pool, size)` | Allocate `size` bytes, uninitialized |
| `arena_calloc(arena_pool, count, size)` | Allocate `count * size` bytes, zero-initialized |
| `arena_destroy(arena_pool)` | Free all blocks and the arena itself |

## Benchmark

Tested on Windows 10, MinGW-w64, 50,000 mixed-size allocations (8 B – 512 B), 5 rounds.

| Allocator | Time (alloc only) | Speedup |
|-----------|-------------------|---------|
| `malloc` + `free` | 41.0 ms | 1.0x (baseline) |
| `calloc` + `free` | 40.2 ms | 1.0x (baseline) |
| `arena_malloc` | 3.9 ms | **10.4x** |
| `arena_calloc` | 15.2 ms | **2.6x** |
| Metric | System (`calloc`+`free`) | Arena |
|--------|--------------------------|-------|
| System calls | 100,000 | 50,001 |
| Internal blocks | N/A | ~3 (~7 MB total) |

> `arena_malloc` is 10.4x faster because it only bumps a pointer — no system calls, no bookkeeping. `arena_calloc` is 2.6x faster than `calloc` even though both zero-initialize; the cost of `memset` dominates, but the arena still wins.

## Design Decisions

### Why linked-list multi-block instead of `realloc`?

`realloc` may move memory to a new address, invalidating all existing pointers. In arena-based workloads (AST nodes, parsed strings, etc.), objects reference each other — pointer invalidation is catastrophic. The linked-list approach guarantees **old pointers always stay valid**.

### Why no `arena_reset`?

In the linked-list model, `reset` (zeroing all block offsets) would cause the arena to reuse old blocks. However, if a future allocation exceeds all existing block capacities, a new larger block is still appended — meaning the block list only grows. Properly reclaiming oversized blocks would require sorting or eviction logic, adding significant complexity for marginal gain. Instead, **`arena_destroy` + `arena_create`** is the recommended lifecycle pattern.

### Why 8-byte alignment?

Covers all common types on 64-bit systems (`double`, `void*`, `int64_t`). The alignment algorithm uses a branchless bitwise operation:

c
static inline size_t align_up(size_t n) {
return ((n + ALIGNMENT - 1) & ~(ALIGNMENT - 1));
}

### Why not a global singleton?

The library originally used a global singleton for simplicity. The current version uses **explicit multi-instance** design — callers create and pass `arena*` explicitly. This enables:
- Thread safety (no shared state)
- Independent lifecycles per subsystem
- Suitability for library code

## Testing

Run the test suite:

bash
gcc -std=c11 -Wall -O2 arena.c test_arena.c -o test_arena && ./test_arena

13 test cases covering:
- Basic allocation & alignment
- Zero-initialization (`arena_calloc`)
- Mixed consecutive allocations
- Overflow protection
- Oversized allocation (triggers new block)
- Multiple instance isolation
- Destroy-one-doesn't-affect-other
- Double-destroy safety
- NULL arena handling

## License

MIT License. See source files for full text.

---

## Future Work

- [ ] `arena_reset` — zero all block offsets without freeing blocks (opt-in)
- [ ] `arena_checkpoint` / `arena_rollback` — scoped allocation with revert
- [ ] JSON parser built on top of this arena (in progress)

---

> Built from scratch with a deep understanding of memory management, not copied from tutorials. 💪
