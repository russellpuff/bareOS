#include <mm/vm.h>
#include <mm/vm_debugger.h>
#include <lib/barelib.h>
#include <lib/bareio.h>

static void format_hex64(char* buffer, uint64_t value) {
    buffer[0] = '0';
    buffer[1] = 'x';
    for (uint32_t i = 0; i < 16; ++i) {
        uint64_t shift = (15 - i) * 4;
        uint8_t nibble = (value >> shift) & 0xF;
        buffer[2 + i] = (nibble < 10) ? ('0' + nibble) : ('A' + (nibble - 10));
    }
    buffer[18] = '\0';
}

static uint64_t canonicalize_sv39(uint64_t va39) {
    const uint64_t sign_bit = 1ULL << 38;
    if (va39 & sign_bit) {
        return va39 | (~0ULL << 39);
    }
    return va39 & ((1ULL << 39) - 1);
}

static uint64_t level_span_bytes(uint32_t level) {
    switch (level) {
    case 2: return 1ULL << 30; /* 1 GiB */
    case 1: return 1ULL << 21; /* 2 MiB */
    default: return 1ULL << 12; /* 4 KiB */
    }
}

static void dump_pt_level(pte_t* table, uint32_t level, uint16_t vpn2, uint16_t vpn1, uint32_t indent);

static void print_indent(uint32_t indent) {
    for (uint32_t i = 0; i < indent; ++i) kprintf(" ");
}

static void dump_pt_entry(uint32_t level, uint16_t vpn2_idx, uint16_t vpn1_idx, uint16_t vpn0_idx,
    pte_t entry, uint32_t indent) {
    bool is_leaf = entry.r || entry.w || entry.x;
    uint64_t span = level_span_bytes(level);
    uint64_t va39_base = ((uint64_t)vpn2_idx << 30)
        | ((uint64_t)((level >= 1) ? vpn1_idx : 0) << 21)
        | ((uint64_t)((level == 0) ? vpn0_idx : 0) << 12);
    uint64_t va_start = canonicalize_sv39(va39_base);
    uint64_t va_end = canonicalize_sv39(va39_base + span - 1);

    char va_start_buf[19];
    char va_end_buf[19];
    char pa_start_buf[19];
    char pa_end_buf[19];
    char ppn_buf[19];
    format_hex64(va_start_buf, va_start);
    format_hex64(va_end_buf, va_end);

    if (is_leaf) {
        uint64_t pa_start = (uint64_t)entry.ppn << PAGE_SHIFT;
        uint64_t pa_end = pa_start + span - 1;
        format_hex64(pa_start_buf, pa_start);
        format_hex64(pa_end_buf, pa_end);

        if (level == 2) {
            print_indent(indent);
            kprintf("[L2 %u] Leaf -> VA[%s-%s] -> PA[%s-%s] flags=(V%u R%u W%u X%u U%u G%u A%u D%u)\n",
                vpn2_idx,
                va_start_buf,
                va_end_buf,
                pa_start_buf,
                pa_end_buf,
                entry.v,
                entry.r,
                entry.w,
                entry.x,
                entry.u,
                entry.g,
                entry.a,
                entry.d);
        }
        else if (level == 1) {
            print_indent(indent);
            kprintf("[L1 %u:%u] Leaf -> VA[%s-%s] -> PA[%s-%s] flags=(V%u R%u W%u X%u U%u G%u A%u D%u)\n",
                vpn2_idx,
                vpn1_idx,
                va_start_buf,
                va_end_buf,
                pa_start_buf,
                pa_end_buf,
                entry.v,
                entry.r,
                entry.w,
                entry.x,
                entry.u,
                entry.g,
                entry.a,
                entry.d);
        }
        else {
            print_indent(indent);
            kprintf("[L0 %u:%u:%u] Leaf -> VA[%s-%s] -> PA[%s-%s] flags=(V%u R%u W%u X%u U%u G%u A%u D%u)\n",
                vpn2_idx,
                vpn1_idx,
                vpn0_idx,
                va_start_buf,
                va_end_buf,
                pa_start_buf,
                pa_end_buf,
                entry.v,
                entry.r,
                entry.w,
                entry.x,
                entry.u,
                entry.g,
                entry.a,
                entry.d);
        }
    }
    else {
        uint64_t child_pa = (uint64_t)entry.ppn << PAGE_SHIFT;
        format_hex64(ppn_buf, (uint64_t)entry.ppn);
        format_hex64(pa_start_buf, child_pa);

        if (level == 2) {
            print_indent(indent);
            kprintf("[L2 %u] Branch -> child L1 at PPN=%s (PA=%s) covering VA[%s-%s] flags=(V%u)\n",
                vpn2_idx,
                ppn_buf,
                pa_start_buf,
                va_start_buf,
                va_end_buf,
                entry.v);
        }
        else if (level == 1) {
            print_indent(indent);
            kprintf("[L1 %u:%u] Branch -> child L0 at PPN=%s (PA=%s) covering VA[%s-%s] flags=(V%u)\n",
                vpn2_idx,
                vpn1_idx,
                ppn_buf,
                pa_start_buf,
                va_start_buf,
                va_end_buf,
                entry.v);
        }
    }
}

static void dump_pt_level(pte_t* table, uint32_t level, uint16_t vpn2, uint16_t vpn1, uint32_t indent) {
    for (uint16_t idx = 0; idx < 512; ++idx) {
        if (!table[idx].v) continue;

        if (level == 2) {
            dump_pt_entry(level, idx, 0, 0, table[idx], indent);
            bool is_leaf = table[idx].r || table[idx].w || table[idx].x;
            if (!is_leaf) {
                dump_pt_level((pte_t*)PPN_TO_KVA(table[idx].ppn), level - 1, idx, 0, indent + 2);
            }
            continue;
        }

        if (level == 1) {
            dump_pt_entry(level, vpn2, idx, 0, table[idx], indent);
            bool is_leaf = table[idx].r || table[idx].w || table[idx].x;
            if (!is_leaf) {
                dump_pt_level((pte_t*)PPN_TO_KVA(table[idx].ppn), level - 1, vpn2, idx, indent + 2);
            }
            continue;
        }

        dump_pt_entry(level, vpn2, vpn1, idx, table[idx], indent);
    }
}

void dump_root_page(const char* label, uint64_t root_ppn) {
    if (!label) label = "unknown";

    const uint32_t entries = 512;
    const uint32_t columns = 32;
    pte_t* l2 = (pte_t*)PPN_TO_KVA(root_ppn);

    kprintf("\n[vm] Root page dump for %s (PPN=%x)\n", label, (uint32_t)root_ppn);
    for (uint32_t i = 0; i < entries; ++i) {
        bool is_valid = l2[i].v;
        bool is_leaf = is_valid && (l2[i].r || l2[i].w || l2[i].x);
        char symbol = '.';
        if (is_leaf) symbol = 'L';
        else if (is_valid) symbol = 'P';
        kprintf("%c", symbol);
        if ((i + 1) % columns == 0) {
            uint32_t start = i + 1 - columns;
            kprintf("  [%u-%u]\n", start, i);
        }
    }

    kprintf("Legend: '.'=empty  P=pointer  L=1GiB leaf\n");

    dump_pt_level(l2, 2, 0, 0, 2);

    kprintf("End of root page dump for %s\n\n", label);
}