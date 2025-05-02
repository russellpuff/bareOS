#include <barelib.h>
#include <malloc.h>
#include <thread.h>

alloc_t* freelist;

/*  Sets the 'freelist' to 'mem_start' and creates  *
 *  a free allocation at that location for the      *
 *  entire heap.                                    */
//--------- This function is complete --------------//
void init_heap(void) {
  freelist = (alloc_t*)&mem_start;
  freelist->size = (uint64)(get_stack(NTHREADS) - &mem_start - sizeof(alloc_t));
  freelist->state = M_FREE;
  freelist->next = NULL;
}

/*  Locates a free block large enough to contain a new    *
 *  allocation of size 'size'.  Once located, remove the  *
 *  block from the freelist and ensure the freelist       *
 *  contains the remaining free space on the heap.        *
 *  Returns a pointer to the newly created allocation     */
void* malloc(uint64 size) {
	if(size == 0 || freelist == NULL) return 0;
	uint64 need = size + sizeof(alloc_t);
	/* Do walk free list for first-fit free memory. */
	alloc_t* prev = NULL;
	alloc_t* curr = freelist;
	while(curr != NULL) {
		uint64 block_total = curr->size + sizeof(alloc_t);
		if(block_total >= need) break;
		prev = curr;
		curr = curr->next;
	}
	if(curr == NULL) return 0;

	const byte MIN_BYTES = 8;
	alloc_t* next;
	if(curr->size + sizeof(alloc_t) - need >= MIN_BYTES + sizeof(alloc_t)) {
		/* Do split. */
		uint64 remaining_bytes = curr->size + sizeof(alloc_t) - need;
		alloc_t* remainder = (alloc_t*)((byte*)curr + need); /* Cast to get a pointer for arithmetic. */
		remainder->size = remaining_bytes - sizeof(alloc_t);
		remainder->state = M_FREE;
		remainder->next = curr->next;
		/* Update free list */
		if(prev == NULL) freelist = remainder;
		else prev->next = remainder;
		next = remainder;
	} else {
		next = curr->next;
	}
	if(prev == NULL) freelist = next;
	else prev->next = next;

	curr->size = size;
	curr->state = M_USED;
	curr->next = NULL; /* Maybe redundant? */

    return (char*)curr + sizeof(alloc_t);
}

/*  Free the allocation at location 'addr'.  If the newly *
 *  freed allocation is adjacent to another free          *
 *  allocation, coallesce the adjacent free blocks into   *
 *  one larger free block.                                */
void free(void* addr) {
	alloc_t* header = (alloc_t*)((byte*)addr - sizeof(alloc_t)); /* Find header. Currently just trusts you didn't pass in a bad pointer. */
	/* Start hunting for its place in the list. */
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
	if(curr && (byte*)header + sizeof(alloc_t) + header->size == (byte*)curr) {
		/* Next segment of memory is also free. */
		header->size += sizeof(alloc_t) + curr->size;
		header->next = curr->next;
	}

	if(prev && (byte*)prev + sizeof(alloc_t) + prev->size == (byte*)header) {
		/* Previous segment of memory is also free. */
		prev->size += sizeof(alloc_t) + header->size;
		prev->next = header->next;
		header = prev; /* Probably redundant. */
	}

	return;
}
