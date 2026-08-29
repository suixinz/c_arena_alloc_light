#include "arena.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ========== Cross-platform high-resolution timing ==========
#ifdef _WIN32
#include <windows.h>
static double now_sec() {
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)freq.QuadPart;
}
#else
#include <time.h>
static double now_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}
#endif

// ========== SETTINGS ==========
static size_t alloc_sizes[] = { 8, 16, 32, 64, 128, 256, 512 };
#define NUM_SIZES (sizeof(alloc_sizes) / sizeof(alloc_sizes[0]))

#define ROUNDS 3

// ========== HELPER: run one benchmark at given scale ==========

typedef double (*bench_fn)(size_t);

// ---- malloc + free ----
static double bench_malloc_free(size_t n) {
    static void* ptrs[500000];  // max 500k
    double start = now_sec();
    for (size_t i = 0; i < n; i++) {
        ptrs[i] = malloc(alloc_sizes[i % NUM_SIZES]);
        if (!ptrs[i]) return -1;
    }
    for (size_t i = n; i-- > 0; ) {
        free(ptrs[i]);
    }
    return now_sec() - start;
}

// ---- calloc + free ----
static double bench_calloc_free(size_t n) {
    static void* ptrs[500000];
    double start = now_sec();
    for (size_t i = 0; i < n; i++) {
        ptrs[i] = calloc(1, alloc_sizes[i % NUM_SIZES]);
        if (!ptrs[i]) return -1;
    }
    for (size_t i = n; i-- > 0; ) {
        free(ptrs[i]);
    }
    return now_sec() - start;
}

// ---- malloc + memset + free ----
static double bench_malloc_memset_free(size_t n) {
    static void* ptrs[500000];
    double start = now_sec();
    for (size_t i = 0; i < n; i++) {
        size_t sz = alloc_sizes[i % NUM_SIZES];
        ptrs[i] = malloc(sz);
        if (!ptrs[i]) return -1;
        memset(ptrs[i], 0, sz);
    }
    for (size_t i = n; i-- > 0; ) {
        free(ptrs[i]);
    }
    return now_sec() - start;
}

// ---- arena_malloc alloc only ----
static double bench_arena_malloc_alloc(size_t n) {
    arena* a = arena_create();
    if (!a) return -1;
    static void* ptrs[500000];
    double start = now_sec();
    for (size_t i = 0; i < n; i++) {
        ptrs[i] = arena_malloc(a, alloc_sizes[i % NUM_SIZES]);
        if (!ptrs[i]) { arena_destroy(a); return -1; }
    }
    double mid = now_sec();
    arena_destroy(a);
    (void)mid;
    return mid - start;
}

// ---- arena_malloc total ----
static double bench_arena_malloc_total(size_t n) {
    arena* a = arena_create();
    if (!a) return -1;
    double start = now_sec();
    for (size_t i = 0; i < n; i++) {
        void* p = arena_malloc(a, alloc_sizes[i % NUM_SIZES]);
        if (!p) { arena_destroy(a); return -1; }
    }
    arena_destroy(a);
    return now_sec() - start;
}

// ---- arena_calloc alloc only ----
static double bench_arena_calloc_alloc(size_t n) {
    arena* a = arena_create();
    if (!a) return -1;
    static void* ptrs[500000];
    double start = now_sec();
    for (size_t i = 0; i < n; i++) {
        ptrs[i] = arena_calloc(a, 1, alloc_sizes[i % NUM_SIZES]);
        if (!ptrs[i]) { arena_destroy(a); return -1; }
    }
    double mid = now_sec();
    arena_destroy(a);
    (void)mid;
    return mid - start;
}

// ---- arena_calloc total ----
static double bench_arena_calloc_total(size_t n) {
    arena* a = arena_create();
    if (!a) return -1;
    double start = now_sec();
    for (size_t i = 0; i < n; i++) {
        void* p = arena_calloc(a, 1, alloc_sizes[i % NUM_SIZES]);
        if (!p) { arena_destroy(a); return -1; }
    }
    arena_destroy(a);
    return now_sec() - start;
}

// ---- arena_reset + reuse ----
static double bench_arena_reset_reuse(size_t n) {
    arena* a = arena_create();
    if (!a) return -1;
    double start = now_sec();
    for (size_t i = 0; i < n; i++) {
        void* p = arena_malloc(a, alloc_sizes[i % NUM_SIZES]);
        if (!p) { arena_destroy(a); return -1; }
    }
    arena_reset(a);
    for (size_t i = 0; i < n; i++) {
        void* p = arena_malloc(a, alloc_sizes[i % NUM_SIZES]);
        if (!p) { arena_destroy(a); return -1; }
    }
    arena_destroy(a);
    return now_sec() - start;
}

// ========== MAIN ==========
int main() {
    size_t scales[] = { 50000, 100000, 500000 };
    int num_scales = sizeof(scales) / sizeof(scales[0]);

    printf("=== Arena vs System Allocator Benchmark (Multi-Scale) ===\n\n");
    printf("Allocation sizes (B):");
    for (int i = 0; i < NUM_SIZES; i++) printf(" %zu", alloc_sizes[i]);
    printf("\nRounds: %d\n\n", ROUNDS);

    for (int s = 0; s < num_scales; s++) {
        size_t n = scales[s];
        printf("========================================\n");
        printf("  Scale: %zu allocations per round\n", n);
        printf("========================================\n\n");

        double t_malloc_free = 0, t_calloc_free = 0, t_malloc_memset = 0;
        double t_am_alloc = 0, t_am_total = 0;
        double t_ac_alloc = 0, t_ac_total = 0;
        double t_reset_reuse = 0;

        for (int r = 0; r < ROUNDS; r++) {
            printf("  Round %d/%d...\n", r + 1, ROUNDS);
            t_malloc_free   += bench_malloc_free(n);
            t_calloc_free   += bench_calloc_free(n);
            t_malloc_memset += bench_malloc_memset_free(n);
            t_am_alloc      += bench_arena_malloc_alloc(n);
            t_am_total      += bench_arena_malloc_total(n);
            t_ac_alloc      += bench_arena_calloc_alloc(n);
            t_ac_total      += bench_arena_calloc_total(n);
            t_reset_reuse   += bench_arena_reset_reuse(n);
        }

        printf("\n  --- Results (%zu allocs × %d rounds) ---\n\n", n, ROUNDS);

        printf("  malloc + free              : %8.3f ms\n", t_malloc_free * 1000);
        printf("  calloc + free              : %8.3f ms\n", t_calloc_free * 1000);
        printf("  malloc+memset + free       : %8.3f ms\n\n", t_malloc_memset * 1000);

        printf("  arena_malloc (alloc only)  : %8.3f ms  (%.1fx)\n",
               t_am_alloc * 1000, t_malloc_free / t_am_alloc);
        printf("  arena_malloc (alloc+free)  : %8.3f ms  (%.1fx)\n\n",
               t_am_total * 1000, t_malloc_free / t_am_total);

        printf("  arena_calloc (alloc only)  : %8.3f ms  (%.1fx)\n",
               t_ac_alloc * 1000, t_calloc_free / t_ac_alloc);
        printf("  arena_calloc (alloc+free)  : %8.3f ms  (%.1fx)\n\n",
               t_ac_total * 1000, t_calloc_free / t_ac_total);

        printf("  arena_reset + reuse        : %8.3f ms  (%.1fx)\n\n",
               t_reset_reuse * 1000, t_malloc_free / t_reset_reuse);

        printf("  --- Call Count ---\n");
        printf("  malloc/free                : %zu calls\n", n * 2 * ROUNDS);
        printf("  arena_malloc+destroy       : %zu + %d\n", n, ROUNDS);
        printf("  arena_reset reuse          : %dx alloc + %d reset + %d destroy\n\n",
               ROUNDS * 2, ROUNDS, ROUNDS);
    }

    printf("✅ All benchmarks complete!\n");
    return 0;
}