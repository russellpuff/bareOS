#include <mm/vm.h>
#include <mm/kmalloc.h>
#include <system/thread.h>
#include <system/panic.h>
#include <system/memlayout.h>
#include <util/string.h>
#include <barelib.h>

/* Allocator does not consider anything below &mem_start allocatable *
 * Since we've enforced boundaries and alignment for all this, we    *
 * just need to manually map reserved sections to the kernel         */
#define FREEMASK_RANGE ((uint64_t)(&mem_end - &mem_start))
#define FREEMASK_BITS (FREEMASK_RANGE / PAGE_SIZE)
#define FREEMASK_SZ ((uint64_t)((FREEMASK_BITS + 7) / 8))
#define PPN_TO_IDX(ppn) (ppn - ((uint64_t)&mem_start >> PAGE_SHIFT))
#define IDX_TO_PPN(idx) (idx + ((uint64_t)&mem_start >> PAGE_SHIFT))

/* Helpers that extract the Sv39 virtual page numbers (VPNs) from a virtual	    *
 * address. Each VPN corresponds to an index within a level of the three-level  *
 * page table hierarchy (L2 -> L1 -> L0).                                       */
static inline uint64_t va_vpn2(uint64_t va) { return (va >> 30) & 0x1ff; }
static inline uint64_t va_vpn1(uint64_t va) { return (va >> 21) & 0x1ff; }
static inline uint64_t va_vpn0(uint64_t va) { return (va >> 12) & 0x1ff; }

/* The freemask is a bitmask used to track what pages are free. 1 byte = 8 bits = 8 pages tracked. *
 * We start tracking pages managed by the allocator at &mem_start, so in order to translate a 	   *
 * physical page number to index, we use the above helpers to move back and forth.                 */
byte* page_freemask;
uint64_t kernel_root_ppn;
byte* s_trap_top;
volatile uint8_t MMU_ENABLED;

//
// Freemask operators
//
/* Takes a ppn and sets it as used in the bitmask */
static void pfm_set(uint64_t ppn) {
	uint64_t x = PPN_TO_IDX(ppn);
	page_freemask[x / 8] |= 0x1 << (x % 8);
}

/* Unused for now, removed bc -Wall -Werror */
// /* Returns whether a ppn is used (1) or free (0) */
// static uint8_t pfm_get(uint64_t ppn) {
// 	uint64_t x = PPN_TO_IDX(ppn);
// 	return (page_freemask[x / 8] >> (x % 8)) & 0x1;
// }

/* Sets a ppn as unused */
static void pfm_clear(uint64_t ppn) {
	uint64_t x = PPN_TO_IDX(ppn);
	page_freemask[x / 8] &= ~(0x1 << (x % 8));
}

/* Returns a free ppn */
static int64_t pfm_findfree_4k(void) {
	for (uint32_t i = 0; i < FREEMASK_SZ; ++i) {
		if (page_freemask[i] != 0xFF) {
			for (uint8_t j = 0; j < 8; ++j) {
				if (((page_freemask[i] >> j) & 0x1) == 0x0) {
					return IDX_TO_PPN((i * 8) + j);
				}
			}
		}
	}
	return NULL;
}

/* Returns a free megapage ppn */
static int64_t pfm_findfree_2m(void) {
	uint8_t chunksz = 64;
	uint64_t limit = (FREEMASK_SZ / chunksz) * chunksz;
	for (uint64_t i = 0; i < limit; i += chunksz) {
		byte* offset = &page_freemask[i];
		uint8_t j = 0;
		for (; j < chunksz; ++j)
			if (offset[j] != 0) break;
		if (j == chunksz)
			return IDX_TO_PPN(i * 8);
	}
	return NULL;
}

/* Marks a megapage starting at x as used. */
static void set_megapage(uint64_t x) {
	for (uint16_t i = 0; i < 512; ++i)
		pfm_set(x + i);
}

/* Clears a megapage starting at x */
static void clear_megapage(uint64_t x) {
	for (uint16_t i = 0; i < 512; ++i)
		pfm_clear(x + i);
}
//
//
//

//
// Page operators
//
/* Zeroes out a page */
static void clean_page(uint64_t ppn) {
	byte* page = (byte*)PPN_TO_KVA(ppn);
	memset(page, 0, PAGE_SIZE);
}


static pte_t make_nonleaf(uint64_t next_ppn) {
	pte_t nl = { 0 }; /* Leave rest as 0 R/W/X */
	nl.ppn = next_ppn;
	nl.v = 1; /* Set as valid */
	return nl;
}

static pte_t make_leaf(uint64_t leaf_ppn, bool R, bool W, bool X, bool G, bool U) {
	pte_t leaf = { 0 };
	leaf.ppn = leaf_ppn;
	leaf.a = 1;
	leaf.d = W ? 1 : 0;
	leaf.r = !!R;
	leaf.w = !!W;
	leaf.x = !!X;
	leaf.g = !!G;
	leaf.u = !!U;
	leaf.v = 1;
	return leaf;
}

/* Checks if there's an L1 that exists for this L2, if not create it */
static pte_t* ensure_l1(uint64_t root_l2_ppn, uint64_t va) {
	pte_t* l2_page = (pte_t*)PPN_TO_KVA(root_l2_ppn);;
	uint64_t idx = va_vpn2(va);
	bool is_leaf = l2_page[idx].v && (l2_page[idx].r || l2_page[idx].w || l2_page[idx].x);

	if (!l2_page[idx].v || is_leaf) {
		if (is_leaf) l2_page[idx] = (pte_t){ 0 };
		uint64_t l1_ppn = pfm_findfree_4k();
		pfm_set(l1_ppn);
		clean_page(l1_ppn);
		l2_page[idx] = make_nonleaf(l1_ppn);
	}

	return (pte_t*)PPN_TO_KVA(l2_page[idx].ppn);
}

/* These mapping functions assume alignment. Kernel is always aligned *
 * Enforcement is done in the allocator                               */

/* Maps a 2M megapage assuming you already have the page */
static void map_2m(uint64_t root_ppn, uint64_t virt_addr, uint64_t phys_addr,
	bool R, bool W, bool X, bool G, bool U) {
	pte_t* l1_page = ensure_l1(root_ppn, virt_addr);
	uint64_t idx = va_vpn1(virt_addr);
	l1_page[idx] = make_leaf(PA_TO_PPN(phys_addr), R, W, X, G, U);
}

/* Maps a regular 4K page assuming you already have it */
static void map_4k(uint64_t root_ppn, uint64_t virt_addr, uint64_t phys_addr,
	bool R, bool W, bool X, bool G, bool U) {
	pte_t* l1_page = ensure_l1(root_ppn, virt_addr);
	uint64_t l1_idx = va_vpn1(virt_addr);
	if (!l1_page[l1_idx].v) {
		uint64_t l0_ppn = pfm_findfree_4k(); 
		pfm_set(l0_ppn); 
		clean_page(l0_ppn);
		l1_page[l1_idx] = make_nonleaf(l0_ppn);
	}
	pte_t* l0 = (pte_t*)PPN_TO_KVA(l1_page[l1_idx].ppn);
	uint64_t i0 = va_vpn0(virt_addr);
	l0[i0] = make_leaf(PA_TO_PPN(phys_addr), R, W, X, G, U);
}

static void clone_page_tables(uint64_t dst_ppn, uint64_t src_ppn, uint8_t level) {
	byte* dst = (byte*)PPN_TO_KVA(dst_ppn);
	byte* src = (byte*)PPN_TO_KVA(src_ppn);

	memcpy(dst, src, PAGE_SIZE);

	if (level == 0) return;

	pte_t* dst_entries = (pte_t*)dst;
	pte_t* src_entries = (pte_t*)src;

	for (uint16_t i = 0; i < 512; ++i) {
		if (!src_entries[i].v) continue;

		bool is_leaf = src_entries[i].r || src_entries[i].w || src_entries[i].x;
		if (is_leaf) continue;

		int64_t child_dst_ppn = pfm_findfree_4k();
		if (child_dst_ppn < 0) return; /* TODO: handle OOM */

		pfm_set(child_dst_ppn);
		clone_page_tables(child_dst_ppn, src_entries[i].ppn, level - 1);
		dst_entries[i].ppn = child_dst_ppn;
	}
}
/* TODO: Don't clone kernel identity map */
static void clone_kernel_map(uint64_t new_root_ppn) {
	clone_page_tables(new_root_ppn, kernel_root_ppn, 2);
}

static void free_l0(uint64_t l0_ppn) {
	pte_t* l0 = (pte_t*)PPN_TO_KVA(l0_ppn);
	for (uint16_t i = 0; i < 512; ++i) {
		if (!l0[i].v) continue;
		if (l0[i].g && (l0[i].r || l0[i].w || l0[i].x)) {
			continue;
		}
		pfm_clear(l0[i].ppn);
	}
	pfm_clear(l0_ppn);
}

static void free_l1(uint64_t l1_ppn) {
	pte_t* l1 = (pte_t*)PPN_TO_KVA(l1_ppn);
	for (uint16_t i = 0; i < 512; ++i) {
		if (!l1[i].v) continue;
		if (l1[i].g && (l1[i].r || l1[i].w || l1[i].x)) {
			continue;
		}
		if (l1[i].r || l1[i].w || l1[i].x) clear_megapage(l1[i].ppn);
		else free_l0(l1[i].ppn);
	}
	pfm_clear(l1_ppn);
}
//
//
//


uint64_t mmu_prepare_process(uint32_t thread_id, thread_mode mode) {
	if (thread_id > NTHREADS) return NULL;

	/* Get root */
	int64_t root_ppn = pfm_findfree_4k();
	pfm_set(root_ppn); clone_kernel_map(root_ppn);

	/* Get singular starting code leaf */
	int64_t code_leaf = pfm_findfree_4k();
	pfm_set(code_leaf);
	map_4k(root_ppn, 0x0UL, PPN_TO_PA(code_leaf), /*R*/1,/*W*/1,/*X*/1,/*G*/0,/*U*/1);

	/* Get singular starting heap leaf */
	int64_t heap_leaf = pfm_findfree_4k();
	pfm_set(heap_leaf);
	map_4k(root_ppn, HEAP_START_VA, PPN_TO_PA(heap_leaf), /*R*/1,/*W*/1,/*X*/0,/*G*/0,/*U*/1);

	/* Get some stack space, 512KiB of stack space for now. Dynamic later. */
	const uint16_t stack_size = 512 * 1024;
	uint16_t n_pages = stack_size / PAGE_SIZE;
	const uint64_t start_addr = KVM_BASE - stack_size;
	for (uint16_t i = 0; i < n_pages; ++i) {
		int64_t ppn = pfm_findfree_4k();
		pfm_set(ppn);
		map_4k(root_ppn, start_addr + (i * PAGE_SIZE), PPN_TO_PA(ppn), /*R*/1,/*W*/1,/*X*/0,/*G*/0,/*U*/1);
	}
	
	/* Get 8KiB kstack for process. Assures leaves are consecutive in lieu of atomic operations */
	int64_t kleaf, kleaf2;
	while (1) {
		kleaf = pfm_findfree_4k();
		pfm_set(kleaf);
		kleaf2 = pfm_findfree_4k();
		pfm_set(kleaf2);
		if (PPN_TO_PA(kleaf) + PAGE_SIZE != PPN_TO_PA(kleaf2)) {
			pfm_clear(kleaf);
			pfm_clear(kleaf2);
		}
		else break;
	}
	/* This is called once before MMU is enabled so context_load has to correct that. */
	thread_table[thread_id].kstack_base = (byte*)PPN_TO_KVA(kleaf); 
	thread_table[thread_id].kstack_top = (byte*)(PPN_TO_KVA(kleaf2) + PAGE_SIZE);
	return root_ppn;
}

/* Rudimentary page clearer. Should be able to free all children of a root page. */
/* Cannot clear a gigapage. We don't have any non-kernel gigapages so who cares. */
void mmu_free_table(uint64_t root_ppn) {
	if (root_ppn == kernel_root_ppn) return;
	pte_t* root = (pte_t*)PPN_TO_KVA(root_ppn);

	for (uint16_t i = 0; i < 512; ++i) {
		if (!root[i].v) continue;
		if (root[i].v && (root[i].g && (root[i].r || root[i].w || root[i].x))) continue; /* Kernel global pte, do not free */

		if (root[i].r || root[i].w || root[i].x) clear_megapage(root[i].ppn);
		else free_l1(root[i].ppn);
	}

	pfm_clear(root_ppn);
}

/* Frees up all the pages owned by a process */
void mmu_free_process(uint32_t thread_id) {
	uint64_t k1 = KVA_TO_PPN(thread_table[thread_id].kstack_base);
	pfm_clear(k1);
	uint64_t k2 = KVA_TO_PPN(thread_table[thread_id].kstack_base + PAGE_SIZE);
	pfm_clear(k2);
	mmu_free_table(thread_table[thread_id].root_ppn);
}

/* This is a key function used to migrate kernel functionality 
 * from a pre-MMU state	to a post-MMU state.                   */
void mmu_migrate_kernel(void) {
	/* s_trap_top is permanently unused after ctxload */
	uint32_t trap_ppn = PA_TO_PPN(s_trap_top - PAGE_SIZE);
	pfm_clear(trap_ppn);
}

/* Function is called when a process heap needs to be enlarged. *
 * We pass in the thread_id of the calling process, or NTHREADS	*
 * if the caller is kmalloc. Returns the number of bytes the	*
 * target heap was expanded by.                                 */
uint32_t mmu_expand_heap(uint32_t request, uint8_t thread_id) {
	if (thread_id > NTHREADS || thread_table[thread_id].state == TH_DEFUNCT) return 0;
	bool is_kernel = thread_id == NTHREADS;
	uint32_t sz;
	uint16_t root_ppn;
	if (is_kernel) { /* Caller is kmalloc */
		sz = k_heap_sz;
		root_ppn = kernel_root_ppn;
	}
	else {
		sz = thread_table[thread_id].heap_sz;
		root_ppn = thread_table[thread_id].root_ppn;
	}
	/* Lazy check to block insane requests. Until we decide realistic process limits, *
	 * attempting to malloc something close to this will just OOM the system          */
	const uint64_t MAX_HEAP = ((uint64_t)&mem_end - (uint64_t)&mem_start);
	if ((uint64_t)sz + request > MAX_HEAP) return 0;

	uint16_t pages_needed = (request + PAGE_SIZE - 1) / PAGE_SIZE;
	uint32_t expanded_by = 0;
	/* One by one attempt to complete the request, exit early if we run out of pages */
	for (int i = 0; i < pages_needed; ++i) {
		int64_t page = pfm_findfree_4k();
		if (page == NULL) break;
		pfm_set(page);
		uint64_t virt_addr = HEAP_START_VA + sz + expanded_by;
		map_4k(root_ppn, virt_addr, PPN_TO_PA(page), /*R*/1,/*W*/1,/*X*/0,/*G*/is_kernel,/*U*/!is_kernel);
		expanded_by += PAGE_SIZE;
	}
	return expanded_by;
}

static uint32_t mmu_expand_code(uint32_t request, uint8_t thread_id) {

}

/* TODO: panic if any ppn is -1 */
/* Initialize the MMU state by preparing the kernel root page */
void init_pages(void) {
	//
	// Setup
	//
	MMU_ENABLED = false;

	page_freemask = kmalloc(FREEMASK_SZ);
	if (page_freemask == NULL) {
		panic("Couldn't kmalloc enough space for the page freemask, cannot init pages.\n");
	}
	memset(page_freemask, 0, FREEMASK_SZ);

	/* Assess that &mem_start is 4K aligned so the MMU can correctly map the kernel */
	if (((uint64_t)&mem_start & (uint64_t)0xFFF) != 0) {
		panic("Fatal: mem_start is not 4KiB aligned.\n");
	}

	//
	// Map the kernel's code (&text_start to &mem_start - 1) identity-style
	// This assures that Supervisor can access kernel functions while running
	// on a user SATP during interrupts.
	//

	/* Create root page for kernel */
	kernel_root_ppn = pfm_findfree_4k();
	if (kernel_root_ppn == NULL) {
		panic("Couldn't find a free page for the kernel root, cannot init pages.\n");
	}
	pfm_set(kernel_root_ppn);
	clean_page(kernel_root_ppn);

	/* TODO: don't identity map the kernel, move the program counter up to the KVA range */
	/* Surgically map the kernel identity-style, positioned between &text_start and &mem_start */
	for (uint64_t page = &text_start; page < &mem_start; page += 0x1000UL) {
		map_4k(kernel_root_ppn, page, page, /*R*/1,/*W*/1,/*X*/1,/*G*/1,/*U*/0);
	}

	//
	// Map all of MMIO and ram virtual-style
	//

	/* Map all of ram virtual-style */
	uint64_t pa = (uint64_t)&text_start & ~((1ULL << 21) - 1);
	uint64_t end = ((uint64_t)&mem_end + ((1ULL << 21) - 1)) & ~((1ULL << 21) - 1);
	for (; pa < end; pa += (1UL << 21)) {
		uint64_t va = KVM_BASE + pa;
		map_2m(kernel_root_ppn, va, pa, /*R*/1,/*W*/1,/*X*/1,/*G*/1,/*U*/0);
	}

	/* Map whole ass MMIO to kernel. Why not? */
	pte_t* l2 = (pte_t*)(PPN_TO_KVA(kernel_root_ppn));
	/* L2 index for 1 GiB slices */
	const uint64_t mmio_va0 = 0x00000000UL + KVM_BASE;
	const uint64_t mmio_va1 = 0x40000000UL + KVM_BASE;
	/* Yeah sure write anywhere to MMIO */
	l2[va_vpn2(mmio_va0)] = make_leaf(PA_TO_PPN(0x00000000UL), /*R*/1,/*W*/1,/*X*/0,/*G*/1,/*U*/0);
	l2[va_vpn2(mmio_va1)] = make_leaf(PA_TO_PPN(0x40000000UL), /*R*/1,/*W*/1,/*X*/0,/*G*/1,/*U*/0);

	/* Get a page for supervisor interrupt stack */
	uint64_t s_trap_ppn = pfm_findfree_4k();
	if (s_trap_ppn == NULL) {
		panic("Couldn't find a free page for the stack trap page, cannot init pages.\n");
	}
	pfm_set(s_trap_ppn);
	clean_page(s_trap_ppn);
	s_trap_top = (byte*)(PPN_TO_PA(s_trap_ppn) + PAGE_SIZE); /* For use by the trap handler */

	/* Get the kernel's starter heap */
	k_heap_sz = 0;
	if (mmu_expand_heap(PAGE_SIZE, NTHREADS) != PAGE_SIZE) {
		panic("Failed to initialize the kernel's virtual heap.\n");
	}
}

//
// Utility functions used by exec()
//

/* Helper function finds the KVA that correlates to a user virtual address */
void* translate_user_va(uint64_t root_ppn, uint64_t va) {
	if (root_ppn == NULL) return NULL;

	pte_t* l2 = (pte_t*)PPN_TO_KVA(root_ppn);
	pte_t pte = l2[va_vpn2(va)];
	if (!pte.v) return NULL;

	if (pte.r || pte.w || pte.x) {
		uint64_t pa = (pte.ppn << PAGE_SHIFT) | (va & ((1ULL << 30) - 1));
		return PA_TO_KVA(pa);
	}

	pte_t* l1 = (pte_t*)PPN_TO_KVA(pte.ppn);
	pte = l1[va_vpn1(va)];
	if (!pte.v) return NULL;

	if (pte.r || pte.w || pte.x) {
		uint64_t pa = (pte.ppn << PAGE_SHIFT) | (va & ((1ULL << 21) - 1));
		return PA_TO_KVA(pa);
	}

	pte_t* l0 = (pte_t*)PPN_TO_KVA(pte.ppn);
	pte = l0[va_vpn0(va)];
	if (!pte.v || !(pte.r || pte.w || pte.x)) return NULL;

	uint64_t pa = (pte.ppn << PAGE_SHIFT) | (va & (PAGE_SIZE - 1));
	return PA_TO_KVA(pa);
}

/* Copies data into user pages */
int32_t copy_to_kva(uint64_t root_ppn, uint64_t va, const void* src, uint64_t len) {
	const byte* src_bytes = (const byte*)src;
	uint64_t remaining = len;

	while (remaining > 0) {
		byte* dst = (byte*)translate_user_va(root_ppn, va);
		if (dst == NULL) return -1;

		uint64_t chunk = PAGE_SIZE - (va & (PAGE_SIZE - 1));
		if (chunk > remaining) chunk = remaining;

		memcpy(dst, src_bytes, chunk);

		va += chunk;
		src_bytes += chunk;
		remaining -= chunk;
	}

	return 0;
}

/* Zeroes out user pages starting at va for len */
int32_t zero_user_pages(uint64_t root_ppn, uint64_t va, uint64_t len) {
	uint64_t remaining = len;

	while (remaining > 0) {
		byte* dst = (byte*)translate_user_va(root_ppn, va);
		if (dst == NULL) return -1;

		uint64_t chunk = PAGE_SIZE - (va & (PAGE_SIZE - 1));
		if (chunk > remaining) chunk = remaining;

		memset(dst, 0, chunk);

		va += chunk;
		remaining -= chunk;
	}

	return 0;
}