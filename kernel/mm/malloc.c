#include <lib/barelib.h>
#include <system/thread.h>
#include <system/panic.h>
#include <mm/malloc.h>
#include <mm/vm.h>

alloc_t* freelist;

/* While the freelist is still fixed and kernel malloc isn't page-aware, we limit its size manually */
#define FREELIST_MAX 16 * 1024 * 1024

void init_heap(void) {
  freelist = (alloc_t*)ALIGN_UP_2M(&mem_start);
  freelist->size = FREELIST_MAX - sizeof(alloc_t);
  freelist->state = M_FREE;
  freelist->next = NULL;
}

/*  Locates a free block large enough to contain a new    *
 *  allocation of size 'size'.  Once located, remove the  *
 *  block from the freelist and ensure the freelist       *
 *  contains the remaining free space on the heap.        *
 *  Returns a pointer to the newly created allocation     */
void* malloc(uint64_t size) {
	if(size == 0 || freelist == NULL || size > FREELIST_MAX - sizeof(alloc_t)) return NULL;
	/* Do walk free list for first-fit free memory. */
	alloc_t* prev = NULL;
	alloc_t* curr = freelist;
	while(curr != NULL) {
		if(curr->size >= size) break;
		prev = curr;
		curr = curr->next;
	}
	if(curr == NULL) return NULL;

	const uint16_t NODE_SZ = sizeof(alloc_t);
	const uint8_t MIN_BYTES = 8 + NODE_SZ;
	alloc_t* next;
	uint64_t remainder_total = curr->size - size;
	if(remainder_total >= MIN_BYTES) {
		/* Do split. */
		alloc_t* remainder = (alloc_t*)((byte*)curr + NODE_SZ + size); /* Cast to get a pointer for arithmetic. */
		remainder->size = remainder_total - NODE_SZ;
		remainder->state = M_FREE;
		remainder->next = curr->next;
		/* Update free list */
		if(prev == NULL) freelist = remainder;
		else prev->next = remainder;
		next = remainder;

		curr->size = size;
	} else {
		next = curr->next;
	}
	if(prev == NULL) freelist = next;
	else prev->next = next;

	curr->state = M_USED;
	curr->next = curr; /* Temporary secret way of verifying the pointer on free */

	return (char*)curr + NODE_SZ;
}

/*  Free the allocation at location 'addr'.  If the newly *
 *  freed allocation is adjacent to another free          *
 *  allocation, coallesce the adjacent free blocks into   *
 *  one larger free block.                                */
void free(void* addr) {
	if(addr == NULL) return;
	
	const uint16_t NODE_SZ = sizeof(alloc_t);
	alloc_t* header = (alloc_t*)((byte*)addr - NODE_SZ); /* Find header. */
	if (header->next != header) {
		panic("Error: 'free' called on a pointer with a nonzero offset.\n");
	}

	/* Find freelist nodes before/after this used segment for coalesce */
	alloc_t* prev = NULL;
	alloc_t* curr = freelist;
	while(curr && curr < header) {
		prev = curr;
		curr = curr->next;
	}
	
	header->state = M_FREE;
	header->next = curr;
	if(prev) prev->next = header;
	else freelist = header;

	/* Try to coalesce. */
	if(curr && (byte*)header + NODE_SZ + header->size == (byte*)curr) {
		/* Next segment of memory is also free. */
		header->size += NODE_SZ + curr->size;
		header->next = curr->next;
	}

	if(prev && (byte*)prev + NODE_SZ + prev->size == (byte*)header) {
		/* Previous segment of memory is also free. */
		prev->size += NODE_SZ + header->size;
		prev->next = header->next;
	}
}
