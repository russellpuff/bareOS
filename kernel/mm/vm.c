#include <mm/vm.h>
#include <mm/malloc.h>
#include <lib/barelib.h>
#include <lib/string.h>
#include <system/thread.h>
#include <system/panic.h>

#define FREEMASK_RANGE ((uint64_t)ALIGN_UP_2M(&mem_end - &text_start))
#define FREEMASK_BITS (FREEMASK_RANGE / PAGE_SIZE)
#define FREEMASK_SZ ((uint64_t)((FREEMASK_BITS + 7) / 8))
#define PPN_TO_IDX(ppn) (ppn - ((uint64_t)&text_start >> PAGE_SHIFT))
#define IDX_TO_PPN(idx) (idx + ((uint64_t)&text_start >> PAGE_SHIFT))

static inline uint64_t va_vpn2(uint64_t va) { return (va >> 30) & 0x1ff; }
static inline uint64_t va_vpn1(uint64_t va) { return (va >> 21) & 0x1ff; }
static inline uint64_t va_vpn0(uint64_t va) { return (va >> 12) & 0x1ff; }

byte* page_freemask;
uint64_t kernel_root_ppn;
byte* s_trap_top;
volatile byte MMU_ENABLED;

//
// Bitmask operators
//
/* Takes a ppn and sets it as used in the bitmask */
static void pfm_set(uint64_t ppn) {
	uint64_t x = PPN_TO_IDX(ppn);
	page_freemask[x / 8] |= 0x1 << (x % 8);
}

/* Unused for now, removed bc -Wall -Werror */
// /* Returns whether a ppn is used (1) or free (0) */
// static byte pfm_get(uint64_t ppn) {
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
			for (byte j = 0; j < 8; ++j) {
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
	byte chunksz = 64;
	uint64_t limit = (FREEMASK_SZ / chunksz) * chunksz;
	for (uint64_t i = 0; i < limit; i += chunksz) {
		byte* offset = &page_freemask[i];
		byte j = 0;
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
static void map_2m(uint64_t root_l2_ppn, uint64_t virt_addr, uint64_t page_addr,
	bool R, bool W, bool X, bool G, bool U) {
	pte_t* l1_page = ensure_l1(root_l2_ppn, virt_addr);
	uint64_t idx = va_vpn1(virt_addr);
	l1_page[idx] = make_leaf(PA_TO_PPN(page_addr), R, W, X, G, U);
}

///* Maps a regular 4K page assuming you already have it */
//static void map_4k(uint64_t root_l2_ppn, uint64_t virt_addr, uint64_t page_addr,
//	bool R, bool W, bool X, bool G, bool U) {
//	pte_t* l1_page = ensure_l1(root_l2_ppn, virt_addr);
//	uint64_t l1_idx = va_vpn1(virt_addr);
//	if (!l1_page[l1_idx].v) {
//		uint64_t l0_ppn = pfm_findfree_4k(); 
//		pfm_set(l0_ppn); 
//		clean_page(l0_ppn);
//		l1_page[l1_idx] = make_nonleaf(l0_ppn);
//	}
//	pte_t* l0 = (pte_t*)PPN_TO_KVA(l1_page[l1_idx].ppn);
//	uint64_t i0 = va_vpn0(virt_addr);
//	l0[i0] = make_leaf(PA_TO_PPN(page_addr), R, W, X, G, U);
//}

static void clone_page_tables(uint64_t dst_ppn, uint64_t src_ppn, byte level) {
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

/* TODO: panic if any ppn is -1 */
void init_pages(void) {
	MMU_ENABLED = false;

	page_freemask = malloc(FREEMASK_SZ);
	if (page_freemask == NULL) {
		panic("Couldn't malloc enough space for the page freemask, cannot init pages.\n");
	}
	memset(page_freemask, 0, FREEMASK_SZ);

	int64_t kernel_leaf_ppn = pfm_findfree_2m(); /* First 2M available is where the kernel lives */
	set_megapage(kernel_leaf_ppn);

	/* Do fixed heap size (for now). It's 16MiB and lives right past the kernel */
	int64_t heap_leaf0_ppn = pfm_findfree_2m();
	set_megapage(heap_leaf0_ppn);
	for (uint32_t i = 1; i < 8; ++i) { int64_t h = pfm_findfree_2m(); set_megapage(h);}

	/* Create root page for kernel */
	kernel_root_ppn = pfm_findfree_4k();
	if (kernel_root_ppn == NULL) {
		panic("Couldn't find a free page for the kernel root, cannot init pages.\n");
	}
	pfm_set(kernel_root_ppn);
	clean_page(kernel_root_ppn);

	/* Map all of ram virtual-style */
	uint64_t pa = (uint64_t)&text_start & ~((1ULL << 21) - 1);
	uint64_t end = ((uint64_t)&mem_end + ((1ULL << 21) - 1)) & ~((1ULL << 21) - 1);
	for (; pa < end; pa += (1UL << 21)) {
		uint64_t va = KVM_BASE + pa;
		map_2m(kernel_root_ppn, va, pa, /*R*/1,/*W*/1,/*X*/1,/*G*/1,/*U*/0);
	}

	/* Map kernel (identity-map style) */
	uint64_t k_virt_addr = (uint64_t)&text_start;
	uint64_t p_page_addr = (uint64_t)PPN_TO_PA(kernel_leaf_ppn);
	map_2m(kernel_root_ppn, k_virt_addr, p_page_addr,/*R*/1,/*W*/1,/*X*/1,/*G*/1,/*U*/0);

	/* Get a page for supervisor interrupt stack */
	uint64_t s_trap_ppn = pfm_findfree_4k();
	if (s_trap_ppn == NULL) {
		panic("Couldn't find a free page for the stack trap page, cannot init pages.\n");
	}
	pfm_set(s_trap_ppn);
	clean_page(s_trap_ppn);
	s_trap_top = (byte*)(PPN_TO_PA(s_trap_ppn) + PAGE_SIZE); /* For use by the trap handler */

	/* Map kernel-heap to kernel root */
	for (uint64_t i = 0; i < 8; ++i) {
		uint64_t hva = k_virt_addr + 0x200000UL * (i + 1);
		uint64_t hpa = (uint64_t)PPN_TO_PA(heap_leaf0_ppn) + (0x200000UL * i);
		map_2m(kernel_root_ppn, hva, hpa, /*R*/1,/*W*/1,/*X*/0,/*G*/1,/*U*/0);
	}

	/* Map whole ass MMIO to kernel. Why not? */
	pte_t* l2 = (pte_t*)(PPN_TO_KVA(kernel_root_ppn));
	/* L2 index for 1 GiB slices */
	const uint64_t mmio_va0 = 0x00000000UL + KVM_BASE;
	const uint64_t mmio_va1 = 0x40000000UL + KVM_BASE;
	/* Yeah sure write anywhere to MMIO */
	l2[va_vpn2(mmio_va0)] = make_leaf(PA_TO_PPN(0x00000000UL), /*R*/1,/*W*/1,/*X*/0,/*G*/1,/*U*/0);
	l2[va_vpn2(mmio_va1)] = make_leaf(PA_TO_PPN(0x40000000UL), /*R*/1,/*W*/1,/*X*/0,/*G*/1,/*U*/0);
}

/* Rudimentary page allocator, allocates a static number of pages and returns  *
 * the root ppn of the newly allocated pages.                                  *
 * Implicitly enforces alignment... for now. Need MVP, will worry later        *
 * Only allocs for processes which are given 4MiB of RAM to work with, for now */
uint64_t alloc_page(uint32_t thread_id) {
	if (thread_id > NTHREADS) return NULL;
	int64_t leaf_ppn;
	int64_t root_ppn = NULL;
	/* Get root */
	root_ppn = pfm_findfree_4k();
	pfm_set(root_ppn); clone_kernel_map(root_ppn);
	/* Get leaves */
	leaf_ppn = pfm_findfree_2m();
	set_megapage(leaf_ppn);
	int64_t leaf2_ppn = pfm_findfree_2m();
	set_megapage(leaf2_ppn);
	/* Map leaves to root */
	map_2m(root_ppn, 0x0UL, (uint64_t)PPN_TO_PA(leaf_ppn), /*R*/1,/*W*/1,/*X*/1,/*G*/0,/*U*/1);
	map_2m(root_ppn, 0x200000UL, (uint64_t)PPN_TO_PA(leaf2_ppn), /*R*/1,/*W*/1,/*X*/0,/*G*/0,/*U*/1);
	/* Get kstack. Assures leaves are consecutive in lieu of atomic operations */
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
	clean_page(kleaf);
	clean_page(kleaf2);
	/* Manual instead of to-KVA func because this is called once before MMU enabled */
	thread_table[thread_id].kstack_base = (byte*)PPN_TO_KVA(kleaf); 
	thread_table[thread_id].kstack_top = (byte*)(PPN_TO_KVA(kleaf2) + PAGE_SIZE);
	return root_ppn;
}

/* Rudimentary page clearer. Should be able to free all children of a root page. */
/* Cannot clear a gigapage. We don't have any non-kernel gigapages so who cares. */
void free_pages(uint64_t root_ppn) {
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

void free_process_pages(uint32_t thread_id) {
	uint64_t k1 = KVA_TO_PPN(thread_table[thread_id].kstack_base);
	pfm_clear(k1);
	uint64_t k2 = KVA_TO_PPN(thread_table[thread_id].kstack_base + PAGE_SIZE);
	pfm_clear(k2);
	free_pages(thread_table[thread_id].root_ppn);
}

/* Helper function finds the KVA that correlates to a user virtual address */
void* translate_user_address(uint64_t root_ppn, uint64_t va) {
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
int32_t copy_to_user(uint64_t root_ppn, uint64_t va, const void* src, uint64_t len) {
	const byte* src_bytes = (const byte*)src;
	uint64_t remaining = len;

	while (remaining > 0) {
		byte* dst = (byte*)translate_user_address(root_ppn, va);
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
int32_t zero_user(uint64_t root_ppn, uint64_t va, uint64_t len) {
	uint64_t remaining = len;

	while (remaining > 0) {
		byte* dst = (byte*)translate_user_address(root_ppn, va);
		if (dst == NULL) return -1;

		uint64_t chunk = PAGE_SIZE - (va & (PAGE_SIZE - 1));
		if (chunk > remaining) chunk = remaining;

		memset(dst, 0, chunk);

		va += chunk;
		remaining -= chunk;
	}

	return 0;
}