#include "arena.h"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

// ========== HELPER ==========

static bool is_aligned(void* ptr, size_t alignment) {
    return ((uintptr_t)ptr & (alignment - 1)) == 0;
}

// ========== TEST ==========

int main() {
    printf("=== Arena Full Functionality Tests ===\n\n");

    // ---- Test 1: arena_create ----
    printf("Test 1: arena_create...");
    arena* a = arena_create();
    assert(a != NULL);
    printf(" PASS\n");

    // ---- Test 2: arena_malloc basic + alignment ----
    printf("Test 2: arena_malloc basic allocation & alignment...");
    int* nums = arena_malloc(a, 5 * sizeof(int));
    assert(nums != NULL);
    assert(is_aligned(nums, ALIGNMENT));
    for (int i = 0; i < 5; i++) nums[i] = i + 1;
    assert(nums[0] == 1 && nums[4] == 5);
    printf(" PASS\n");

    // ---- Test 3: arena_calloc zero init ----
    printf("Test 3: arena_calloc zero initialization...");
    char* buf = arena_calloc(a, 32, sizeof(char));
    assert(buf != NULL);
    assert(is_aligned(buf, ALIGNMENT));
    for (int i = 0; i < 32; i++) assert(buf[i] == 0);
    printf(" PASS\n");

    // ---- Test 4: Mixed consecutive allocations aligned ----
    printf("Test 4: Mixed consecutive allocations aligned...");
    char*   c1 = arena_malloc(a, 1);
    double* d1 = arena_malloc(a, sizeof(double));
    short*  s1 = arena_malloc(a, sizeof(short));
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

    // ---- Test 6: Oversized alloc triggers new block ----
    printf("Test 6: Oversized alloc triggers new block...");
    size_t huge = 2 * 1024 * 1024; // 2MB, over default 1MB
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
    assert(b != NULL && b != a);

    int* from_b = arena_calloc(b, 3, sizeof(int));
    assert(from_b != NULL);
    from_b[0] = 100; from_b[1] = 200; from_b[2] = 300;

    // a's data still intact
    assert(nums[0] == 1 && nums[4] == 5);
    assert(*c1 == 'A');

    // b's data correct
    assert(from_b[0] == 100 && from_b[2] == 300);
    printf(" PASS\n");

    // ---- Test 8: Destroy one, other survives ----
    printf("Test 8: Destroy one instance, other survives...");
    arena_destroy(a);
    from_b[1] = 999;
    assert(from_b[1] == 999);
    arena_destroy(b);
    printf(" PASS\n");

    // ---- Test 9: arena_destroy(NULL) safe ----
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

    // ---- Test 11: arena_malloc(NULL, ...) returns NULL ----
    printf("Test 11: arena_malloc(NULL, 4) returns NULL...");
    assert(arena_malloc(NULL, 4) == NULL);
    printf(" PASS\n");

    // ---- Test 12: arena_calloc(NULL, ...) returns NULL ----
    printf("Test 12: arena_calloc(NULL, 1, 4) returns NULL...");
    assert(arena_calloc(NULL, 1, 4) == NULL);
    printf(" PASS\n");

    // ---- Test 13: arena_malloc size 0 ----
    printf("Test 13: arena_malloc with size 0...");
    arena* d = arena_create();
    assert(d != NULL);
    void* zero = arena_malloc(d, 0);
    (void)zero; // behavior: may return NULL or dummy; just shouldn't crash
    arena_destroy(d);
    printf(" PASS\n");

    // ---- Test 14: arena_reset basic ----
    printf("Test 14: arena_reset basic reuse...");
    arena* e = arena_create();
    assert(e != NULL);
    int* p1 = arena_malloc(e, sizeof(int));
    assert(p1 != NULL);
    *p1 = 777;
    arena_reset(e);
    // after reset, allocate again — should get same or reused memory
    int* p2 = arena_malloc(e, sizeof(int));
    assert(p2 != NULL);
    *p2 = 888;
    // p1 is now invalid (dangling), but writing through p2 is fine
    assert(*p2 == 888);
    arena_destroy(e);
    printf(" PASS\n");

    // ---- Test 15: arena_reset(NULL) safe ----
    printf("Test 15: arena_reset(NULL) is safe...");
    arena_reset(NULL);
    printf(" PASS\n");

    // ---- Test 16: arena_reset + multi-block reuse ----
    printf("Test 16: arena_reset reuses multi-block arena...");
    arena* f = arena_create();
    assert(f != NULL);
    // allocate something small
    int* small = arena_malloc(f, sizeof(int));
    assert(small != NULL);
    *small = 111;
    // allocate something huge → triggers new block
    void* huge2 = arena_malloc(f, 2 * 1024 * 1024);
    assert(huge2 != NULL);
    // reset
    arena_reset(f);
    // after reset, should be able to allocate again from first block
    int* after_reset = arena_calloc(f, 1, sizeof(int));
    assert(after_reset != NULL);
    assert(*after_reset == 0); // zero-initialized
    arena_destroy(f);
    printf(" PASS\n");

    // ---- Test 17: arena_reset invalidates old pointers (semantic) ----
    printf("Test 17: arena_reset invalidates old pointers (semantic)...");
    arena* g = arena_create();
    assert(g != NULL);
    char* str = arena_malloc(g, 16);
    assert(str != NULL);
    memcpy(str, "hello", 6);
    arena_reset(g);
    // str is now invalid — allocating something else should not crash
    char* str2 = arena_malloc(g, 8);
    assert(str2 != NULL);
    memcpy(str2, "world", 6);
    arena_destroy(g);
    printf(" PASS\n");

    printf("\n✅ All 17 tests passed!\n");
    return 0;
}