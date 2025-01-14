#include <barelib.h>
#include <thread.h>
#include <malloc.h>
#include "testing.h"

static tests_t tests[] = {
  {
    .category = "General",
    .prompts = {
      "Program Compiles",
      "Heap initialized"
    }
  },
  {
    .category = "Malloc",
    .prompts = {
      "Allocation removed from free list",
      "Returns correct address",
      "Multiple allocations",
      "Unsatisfiable allocation returns NULL",
      "All heap space can be allocated",
      "New allocations possible after free",
      "New allocations can perfectly fit in free block"
    }
  },
  {
    .category = "Free",
    .prompts = {
      "Freeing single allocation modifies free list",
      "Freeing second allocation modifies free list",
      "Freeing most recent allocation modifies free list",
      "Freelist reorders to maintain order",
      "Allocation coalesces upward",
      "Allocation coalesces downward",
      "Allocation coalesces in both directions"
    }
  },
};

#define REAL_SIZE(x) (sizeof(alloc_t) + x)

extern alloc_t* freelist;

static alloc_t* lastfree;
static void validate_list(int32 phase, int32 test, alloc_t* head, byte** expected, uint32* sizes, uint32 len) {
  for (int i=0; i<len; i++) {
    if (assert(head != NULL, tests[phase].results[test], "s", "FAIL - freelist points to NULL prematurely")) {
      alloc_t* alloc = ((alloc_t*)(expected[i])) - 1;
      assert(head == alloc, tests[phase].results[test], "s", "FAIL - List out of order or includes invalid ptr");
      assert(head->size == sizes[i], tests[phase].results[test], "s", "FAIL - Free block in list is incorrect size");
      assert(head->state == M_FREE, tests[phase].results[test], "s", "FAIL - Free block was not set to FREE state");
      head = head->next;
    }
  }
  assert(head == lastfree, tests[phase].results[test], "s", "FAIL - freelist did not end at the expected address");
}


static byte validate_alloc(uint64 size, uint64 remaining, byte* ptr, byte* expected) {
  byte valid = 1;
  alloc_t* alloc = ((alloc_t*)ptr) - 1;
  if (ptr == NULL)
    return 0;
  
  valid &= assert(ptr == expected, tests[1].results[1], "s", "FAIL - Returned pointer did not match expected address");
  valid &= assert(alloc->size == size, tests[1].results[1], "s", "FAIL - Returned allocation is not marked with the correct size");
  valid &= assert(alloc->state == M_USED, tests[1].results[1], "s", "FAIL - Returned allocation is not set to the correct state");
  if (remaining) {
    valid &= assert(freelist == (alloc_t*)(ptr + size), tests[1].results[0], "s", "FAIL - Freelist not moved to correct offset after alloc");
    valid &= assert(freelist->size > (remaining - 1), tests[1].results[0], "s", "FAIL - Allocation removed too many bytes from freelist");
    valid &= assert(freelist->size < (remaining + 1), tests[1].results[0], "s", "FAIL - Allocation removed too few bytes from freelist");
  }
  else
    valid &= assert(freelist == NULL, "s", "FAIL - Freelist was not set to NULL on empty heap");
  return valid;
}

static uint32 disabled(void) {
  return 1;
}

static uint32 setup_general(void) {
  uint64 initial_size = (uint64)(get_stack(NTHREADS) - &mem_start - sizeof(alloc_t));
  assert(1, tests[0].results[0], "");
  assert(freelist->state == M_FREE, tests[0].results[1], "s", "FAIL - Initial freelist state is not set to free");
  assert(freelist->next == NULL, tests[0].results[1], "s", "FAIL - Initial freelist next pointer not set to NULL");
  assert(freelist->size == initial_size, tests[0].results[1], "s", "FAIL - Initial freelist size is not full available size");
  
  t__reset();
  return 1;
}

static uint32 setup_malloc(void) {
  alloc_t* rel = freelist;
  byte* ptr[5];
  uint64 reserved = 0, heapsz = freelist->size;

  ptr[0] = malloc(5);
  reserved += REAL_SIZE(5);
  validate_alloc(5, heapsz - reserved, ptr[0], (byte*)(rel + 1));

  rel = freelist;
  ptr[1] = malloc(1024);
  reserved += REAL_SIZE(1024);
  assert(validate_alloc(1024, heapsz - reserved, ptr[1], (byte*)(rel + 1)), tests[1].results[2],
	 "s", "FAIL - Second allocation was not correctly reserved");

  rel = freelist;
  ptr[2] = malloc(5000);
  reserved += REAL_SIZE(5000);
  assert(validate_alloc(5000, heapsz - reserved, ptr[2], (byte*)(rel + 1)), tests[1].results[2],
	 "s", "FAIL - Extra allocations were not correctly reserved");

  rel = freelist;
  ptr[3] = malloc(400);
  reserved += REAL_SIZE(400);
  assert(validate_alloc(400, heapsz - reserved, ptr[3], (byte*)(rel + 1)), tests[1].results[2],
	 "s", "FAIL - Extra allocations were not correctly reserved");

  rel = freelist;
  ptr[4] = malloc(heapsz);
  assert(ptr[4] == NULL, tests[1].results[3], "s", "FAIL - Allocation larger than remaining size returned non-NULL");

  rel = freelist;
  ptr[4] = malloc(heapsz - reserved);
  assert(validate_alloc(heapsz - reserved, 0, ptr[4], (byte*)(rel + 1)), tests[1].results[4],
	 "s", "FAIL - Allocating all remaining space failed");
  
  byte* badptr = malloc(5);
  assert(badptr == NULL, tests[1].results[3], "s", "FAIL - Allocation returned when mallocing bytes after heap is exhausted");
  
  t__reset();
  return 1;
}

static uint32 setup_free(void) {
  alloc_t* base = freelist;
  byte* ptr[10];
  uint32 sizes[] = { 400, 12, 82, 1024, 38, 555, 7000, 80000, 40, 1234 };

  for (int i=0; i<10; i++)
    ptr[i] = malloc(sizes[i]);

  lastfree = freelist;

  free(ptr[2]);
  validate_list(2, 0, freelist, (byte*[]){ ptr[2] }, (uint32[]){ sizes[2] }, 1);
  
  free(ptr[7]);
  validate_list(2, 1, freelist, (byte*[]){ ptr[2], ptr[7] }, (uint32[]){ sizes[2], sizes[7] }, 2);
  
  free(ptr[0]);
  validate_list(2, 3, freelist, (byte*[]){ ptr[0], ptr[2], ptr[7] }, (uint32[]){ sizes[0], sizes[2], sizes[7] }, 3);

  free(ptr[6]);
  uint32 new_sizes[3];
  new_sizes[0] = sizes[0];
  new_sizes[1] = sizes[2];
  new_sizes[2] = sizes[6] + REAL_SIZE(sizes[7]);
  validate_list(2, 4, freelist, (byte*[]){ ptr[0], ptr[2], ptr[6] }, new_sizes, 3);

  free(ptr[3]);
  new_sizes[1] += REAL_SIZE(sizes[3]);
  validate_list(2, 5, freelist, (byte*[]){ ptr[0], ptr[2], ptr[6] }, new_sizes, 3);

  free(ptr[1]);
  new_sizes[0] += REAL_SIZE(new_sizes[1]) + REAL_SIZE(sizes[1]);
  new_sizes[1] = new_sizes[2];
  validate_list(2, 6, freelist, (byte*[]){ ptr[0], ptr[6] }, new_sizes, 2);

  free(ptr[9]);
  new_sizes[2] = sizes[9];
  lastfree = ((alloc_t*)ptr[9]) - 1;
  validate_list(2, 2, freelist, (byte*[]){ ptr[0], ptr[6] }, new_sizes, 2);

  ptr[0] = malloc(100);
  // Validate
  alloc_t* alloc = ((alloc_t*)ptr[0]) - 1;
  assert(alloc->size == 100, tests[1].results[5], "s", "FAIL - Reallocated block was incorrect size");
  assert(alloc->state == M_USED, tests[1].results[5], "s", "FAIL - Reallocated block was not set to USED");
  assert(alloc == base, tests[1].results[5], "s", "FAIL - New allocation was not placed in newly freed block");

  free(ptr[0]);
  validate_list(2, 0, freelist, (byte*[]){ ptr[0], ptr[6] }, new_sizes, 2);

  uint64 new_size = base->size;
  ptr[0] = malloc(base->size);
  new_sizes[0] = new_sizes[1];
  alloc = ((alloc_t*)ptr[0]) - 1;
  assert(alloc->size == new_size, tests[1].results[6], "s", "FAIL - Reallocated block was incorrect size");
  assert(alloc->state == M_USED, tests[1].results[6], "s", "FAIL - Reallocated block was not set to USED");
  assert(alloc == base, tests[1].results[6], "s", "FAIL - New allocation was not placed in newly freed block");
  validate_list(1, 6, freelist, (byte*[]){ ptr[6] }, new_sizes, 1);
  
  t__reset();
  return 1;
}

void t__ms7(uint32 run) {
  switch (run) {
  case 0:
    t__initialize_tests(tests, 3);
    t__analytics[RESCHED_REF].precall = (uint32 (*)())disabled;
    t__analytics[DELEG_CLK_REF].precall = (uint32 (*)())disabled;
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_general; break;
  case 1:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_malloc; break;
  case 2:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_free; break;
  }
}
