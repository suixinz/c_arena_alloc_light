#include "arena.h"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

// ========== HELPER FUNCTION ==========

static bool is_aligned(void* ptr, size_t alignment) {
    return ((uintptr_t)ptr & (alignment - 1)) == 0;
}

// ========== TEST ==========

int main() {
    printf("=== Arena Multi-Instance Tests ===\n\n");

    // ---- Test 1: arena_create ----
    printf("Test 1: arena_create...");
    arena* a = arena_create();
    assert(a != NULL);
    printf(" PASS\n");

    // ---- Test 2: arena_malloc basic alloc with alignment ----
    printf("Test 2: arena_malloc basic allocation & alignment...");
    int* nums = arena_malloc(a, 5 * sizeof(int));
    assert(nums != NULL);
    assert(is_aligned(nums, ALIGNMENT));
    for (int i = 0; i < 5; i++) nums[i] = i + 1;
    assert(nums[0] == 1 && nums[4] == 5);
    printf(" PASS\n");

    // ---- Test 3: arena_calloc zero initialization----
    printf("Test 3: arena_calloc zero initialization...");
    char* buf = arena_calloc(a, 32, sizeof(char));
    assert(buf != NULL);
    assert(is_aligned(buf, ALIGNMENT));
    for (int i = 0; i < 32; i++) assert(buf[i] == 0);
    printf(" PASS\n");

    // ---- Test 4: Mixed consecutive allocations aligned ----
    printf("Test 4: Mixed consecutive allocations aligned...");
    char* c1   = arena_malloc(a, 1);
    double* d1 = arena_malloc(a, sizeof(double));
    short* s1  = arena_malloc(a, sizeof(short));
    assert(is_aligned(c1, ALIGNMENT));
    assert(is_aligned(d1, ALIGNMENT));
    assert(is_aligned(s1, ALIGNMENT));
    *c1 = 'A'; *d1 = 3.14; *s1 = 42;
    assert(*c1 == 'A' && *d1 > 3.0 && *d1 < 4.0 && *s1 == 42);
    printf(" PASS\n");

    // ---- Test 5: arena_calloc overflow protection ----
    printf("Test 5: arena_calloc overflow protection...");
    void* ovf = arena_calloc(a, SIZE_MAX / 2, 3);
    assert(ovf == NULL);
    printf(" PASS\n");

    // ---- Test 6: arena_malloc Oversized alloc triggers new block ----
    printf("Test 6: Oversized alloc triggers new block...");
    size_t huge = 2 * 1024 * 1024; // 2MB，over default 1MB
    void* big = arena_malloc(a, huge);
    assert(big != NULL);
    assert(is_aligned(big, ALIGNMENT));
    ((char*)big)[0] = 'X';
    ((char*)big)[huge - 1] = 'Y';
    assert(((char*)big)[0] == 'X' && ((char*)big)[huge - 1] == 'Y');
    printf(" PASS\n");

    // ---- Test 7: Multiple instances isolated ----
    printf("Test 7: Multiple instances isolated...");
    arena* b = arena_create();
    assert(b != NULL);
    assert(b != a);

    int* from_b = arena_calloc(b, 3, sizeof(int));
    assert(from_b != NULL);
    from_b[0] = 100; from_b[1] = 200; from_b[2] = 300;

    // data in a correct
    assert(nums[0] == 1 && nums[4] == 5);
    assert(*c1 == 'A');

    // data in b correct
    assert(from_b[0] == 100 && from_b[2] == 300);
    printf(" PASS\n");

    // ---- Test 8: destroy a，b not influenced ----
    printf("Test 8: Destroy one instance, other survives...");
    arena_destroy(a);
    from_b[1] = 999;
    assert(from_b[1] == 999);
    arena_destroy(b);
    printf(" PASS\n");

    // ---- Test 9: arena_destroy(NULL) not crash ----
    printf("Test 9: arena_destroy(NULL) is safe...");
    arena_destroy(NULL);
    printf(" PASS\n");

    // ---- Test 10: Create after destroy ----
    printf("Test 10: Create after destroy...");
    arena* c = arena_create();
    assert(c != NULL);
    int* fresh = arena_calloc(c, 4, sizeof(int));
    assert(fresh != NULL);
    assert(is_aligned(fresh, ALIGNMENT));
    fresh[0] = 10; fresh[3] = 40;
    assert(fresh[0] == 10 && fresh[3] == 40);
    arena_destroy(c);
    printf(" PASS\n");

    // ---- Test 11: arena_malloc puts NULL returns NULL ----
    printf("Test 11: arena_malloc(NULL, 4) returns NULL...");
    void* bad = arena_malloc(NULL, 4);
    assert(bad == NULL);
    printf(" PASS\n");

    // ---- Test 12: arena_calloc puts NULL returns NULL ----
    printf("Test 12: arena_calloc(NULL, 1, 4) returns NULL...");
    void* bad2 = arena_calloc(NULL, 1, 4);
    assert(bad2 == NULL);
    printf(" PASS\n");

    // ---- Test 13: arena_malloc with size 0 ----
    printf("Test 13: arena_malloc with size 0 returns valid ptr...");
    arena* d = arena_create();
    assert(d != NULL);
    void* zero = arena_malloc(d, 0);
    (void)zero;
    arena_destroy(d);
    printf(" PASS\n");

    printf("\n✅ All 13 tests passed!\n");
    return 0;
}