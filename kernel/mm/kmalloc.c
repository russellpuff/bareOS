#include <system/memlayout.h>
#include <system/thread.h>
#include <system/panic.h>
#include <mm/kmalloc.h>
#include <mm/vm.h>
#include <barelib.h>

alloc_t* freelist;
uint32_t k_heap_sz;

/* Initial freelist has a fixed size based on the provided reserved space */
void init_heap(void) {
	if (MMU_ENABLED) return;
	freelist = &krsvd;
	freelist->size = (&mem_start - &krsvd) / 2;
	freelist->state = M_FREE;
	freelist->next = NULL;
}

/* This regenerates the kernel heap after MMU is turned on. *
 * WARNING: THIS DELETES THE ORIGINAL HEAP'S RECORDS!		*
 * THEY WILL NOT BE AUTOMATICALLY TRANSFERRED, THEY CANNOT	*
 * BE AUTOMATICALLY TRANSFERRED.                            */
void regen_kernel_heap(void) {
	if (!MMU_ENABLED) return;
	freelist = HEAP_START_VA; /* Start at the kernel's virtual heap */
	freelist->size = PAGE_SIZE - sizeof(alloc_t); /* All virtual heaps start out at 1 page, grow when kmalloc */
	freelist->state = M_FREE;
	freelist->next = NULL;
	k_heap_sz = PAGE_SIZE;
}

/*  Locates a free block large enough to contain a new   *
 *  allocation of size 'size'.  Once located, remove the *
 *  block from the freelist and ensure the freelist      *
 *  contains the remaining free space on the heap.       *
 *  Returns a pointer to the newly created allocation.   *
 *  Obviously don't call this before the kernel inits	 *
 *  the freelist. After MMU is on, we switch from the 	 *
 *  bootstrap heap to the kernel's virtual heap, which	 *
 *  regenerates new heap of size 'PAGE_SIZE' bytes. 	 *
 *  Make sure you call 'mmu_migrate_kernel' after MMU is *
 *  on to port any key allocations made before MMU init. */
void* kmalloc(uint32_t size) {
	if(size == 0 || freelist == nullptr) return nullptr;
	/* Do walk free list for first-fit free memory. */
	alloc_t* prev = nullptr;
	alloc_t* curr = nullptr;
	while (1) {
		prev = nullptr;
		curr = freelist;

		while (curr != nullptr) {
			if (curr->size >= size) break;
			prev = curr;
			curr = curr->next;
		}
		if (curr == nullptr) {
			if (!MMU_ENABLED) return nullptr;
			uint32_t addl = mmu_expand_heap(size, NTHREADS); /* Ask MMU for more pages to cover request */
			if (addl == 0) return nullptr;
			/* Expand freelist by converting the raw space into a 'used' block and then freeing it for auto coalesce */
			alloc_t* new_region = (alloc_t*)(HEAP_START_VA + (uint64_t)k_heap_sz);
			new_region->next = new_region;
			new_region->size = addl - sizeof(alloc_t);
			new_region->state = M_USED;
			byte* free_target = (byte*)new_region + sizeof(alloc_t);
			free(free_target);
			k_heap_sz += addl;
			/* When this block finishes, we'll loop again to find the newly freed range and malloc it */
		}
		else break;
	}

	const uint16_t NODE_SZ = sizeof(alloc_t);
	const uint16_t MIN_BYTES = 8 + NODE_SZ;
	alloc_t* next;
	uint64_t remainder_total = curr->size - size;
	if(remainder_total >= MIN_BYTES) {
		/* Do split. */
		alloc_t* remainder = (alloc_t*)((byte*)curr + NODE_SZ + size); /* Cast to get a pointer for arithmetic. */
		remainder->size = remainder_total - NODE_SZ;
		remainder->state = M_FREE;
		remainder->next = curr->next;
		/* Update free list */
		if(prev == nullptr) freelist = remainder;
		else prev->next = remainder;
	} else {
		next = curr->next;
	}
	if(prev == nullptr) freelist = next;
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
