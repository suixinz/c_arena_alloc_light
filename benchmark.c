#include "arena.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ========== 	Cross-platform high-resolution timing ==========
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
#define NUM_ALLOCS   50000
#define ROUNDS       5

// Simulating real-world scenarios
static size_t alloc_sizes[] = { 8, 16, 32, 64, 128, 256, 512 };

#define NUM_SIZES (sizeof(alloc_sizes) / sizeof(alloc_sizes[0]))

// ========== Benchmark A: malloc + free ==========
static double bench_malloc_free(){

    void* ptrs[NUM_ALLOCS];
    double start = now_sec();

    for (int i = 0; i < NUM_ALLOCS; i++){

        ptrs[i] = malloc(alloc_sizes[i % NUM_SIZES]);
        if (!ptrs[i]){
            return -1;
        }

    }

    for (int i = NUM_ALLOCS - 1; i >= 0; i--){
        free(ptrs[i]);
    }

    double end = now_sec();

    return end - start;

}

// ========== Benchmark B: calloc + free ==========
static double bench_calloc_free(){

    void* ptrs[NUM_ALLOCS];
    double start = now_sec();

    for (int i = 0; i < NUM_ALLOCS; i++){

        ptrs[i] = calloc(1, alloc_sizes[i % NUM_SIZES]);
        if (!ptrs[i]){
            return -1;
        }

    }
    for (int i = NUM_ALLOCS - 1; i >= 0; i--){
        free(ptrs[i]);
    }
    double end = now_sec();

    return end - start;

}

// ========== Benchmark C: malloc + memset(0) + free ==========
static double bench_malloc_memset_free(){

    void* ptrs[NUM_ALLOCS];
    double start = now_sec();

    for (int i = 0; i < NUM_ALLOCS; i++){

        size_t sz = alloc_sizes[i % NUM_SIZES];

        ptrs[i] = malloc(sz);
        if (!ptrs[i]){
            return -1;
        }

        memset(ptrs[i], 0, sz);

    }
    for (int i = NUM_ALLOCS - 1; i >= 0; i--){
        free(ptrs[i]);
    }
    double end = now_sec();

    return end - start;

}

// ========== Benchmark D: arena_malloc alloc only ==========
static double bench_arena_malloc_alloc(){

    arena* a = arena_create();
    if (!a) return -1;

    void* ptrs[NUM_ALLOCS];
    double start = now_sec();

    for (int i = 0; i < NUM_ALLOCS; i++){

        ptrs[i] = arena_malloc(a, alloc_sizes[i % NUM_SIZES]);
        if (!ptrs[i]) { arena_destroy(a); return -1; }

    }

    double mid = now_sec();
    arena_destroy(a);
    (void)mid;

    return mid - start;

}

// ========== Benchmark E: arena_malloc alloc + destroy total ==========
static double bench_arena_malloc_total(){

    arena* a = arena_create();
    if (!a) return -1;

    double start = now_sec();

    for (int i = 0; i < NUM_ALLOCS; i++){

        void* p = arena_malloc(a, alloc_sizes[i % NUM_SIZES]);
        if (!p) { arena_destroy(a); return -1; }

    }
    arena_destroy(a);
    double end = now_sec();

    return end - start;

}

// ========== Benchmark F: arena_calloc alloc only ==========
static double bench_arena_calloc_alloc(){

    arena* a = arena_create();
    if (!a) return -1;

    void* ptrs[NUM_ALLOCS];
    double start = now_sec();

    for (int i = 0; i < NUM_ALLOCS; i++){

        ptrs[i] = arena_calloc(a, 1, alloc_sizes[i % NUM_SIZES]);
        if (!ptrs[i]) { arena_destroy(a); return -1; }

    }

    double mid = now_sec();
    arena_destroy(a);
    (void)mid;

    return mid - start;

}

// ========== Benchmark G: arena_calloc alloc + destroy total ==========
static double bench_arena_calloc_total(){

    arena* a = arena_create();
    if (!a) return -1;

    double start = now_sec();
    for (int i = 0; i < NUM_ALLOCS; i++) {
        void* p = arena_calloc(a, 1, alloc_sizes[i % NUM_SIZES]);
        if (!p) { arena_destroy(a); return -1; }
    }

    arena_destroy(a);
    double end = now_sec();

    return end - start;

}

// ========== MAIN FUNCTION ==========
int main(){

    printf("=== Arena vs System Allocator Benchmark ===\n\n");

    printf("Config:\n");
    printf("  Allocations per round : %d\n", NUM_ALLOCS);
    printf("  Allocation sizes (B)  :");
    for (int i = 0; i < NUM_SIZES; i++) {
        printf(" %zu", alloc_sizes[i]);
    }
    printf("\n  Rounds                : %d\n\n", ROUNDS);

    double t_malloc_free = 0;
    double t_calloc_free = 0;
    double t_malloc_memset = 0;
    double t_am_alloc = 0, t_am_total = 0;
    double t_ac_alloc = 0, t_ac_total = 0;

    for (int r = 0; r < ROUNDS; r++){

        t_malloc_free    += bench_malloc_free();
        t_calloc_free    += bench_calloc_free();
        t_malloc_memset  += bench_malloc_memset_free();
        t_am_alloc       += bench_arena_malloc_alloc();
        t_am_total       += bench_arena_malloc_total();
        t_ac_alloc       += bench_arena_calloc_alloc();
        t_ac_total       += bench_arena_calloc_total();

    }

    printf("--- Results (total over %d rounds, %d allocs each) ---\n\n",
           ROUNDS, NUM_ALLOCS);

    printf("  malloc + free              : %8.3f ms\n",
           t_malloc_free * 1000);
    printf("  calloc + free              : %8.3f ms\n",
           t_calloc_free * 1000);
    printf("  malloc+memset + free       : %8.3f ms\n\n",
           t_malloc_memset * 1000);

    printf("  arena_malloc (alloc only)  : %8.3f ms  (%.1fx vs malloc)\n",
           t_am_alloc * 1000, t_malloc_free / t_am_alloc);
    printf("  arena_malloc (alloc+free)  : %8.3f ms  (%.1fx vs malloc)\n\n",
           t_am_total * 1000, t_malloc_free / t_am_total);

    printf("  arena_calloc (alloc only)  : %8.3f ms  (%.1fx vs calloc)\n",
           t_ac_alloc * 1000, t_calloc_free / t_ac_alloc);
    printf("  arena_calloc (alloc+free)  : %8.3f ms  (%.1fx vs calloc)\n\n",
           t_ac_total * 1000, t_calloc_free / t_ac_total);

    printf("--- System Call Count ---\n");
    printf("  malloc/calloc + free       : %d calls\n", NUM_ALLOCS * 2);
    printf("  arena_malloc + destroy     : %d arena_malloc + 1 destroy\n",
           NUM_ALLOCS);
    printf("  arena_calloc + destroy     : %d arena_calloc + 1 destroy\n",
           NUM_ALLOCS);
    printf("  (internal blocks: ~3, total ~7MB)\n");

    printf("\n✅ Benchmark complete!\n");
    return 0;
}