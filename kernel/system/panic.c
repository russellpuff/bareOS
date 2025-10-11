#include <lib/barelib.h>
#include <lib/bareio.h>
#include <system/panic.h>

static inline uint64_t r_sstatus(void) { uint64_t x; asm volatile("csrr %0, sstatus":"=r"(x)); return x; }
static inline uint64_t r_scause(void) { uint64_t x; asm volatile("csrr %0, scause":"=r"(x)); return x; }
static inline uint64_t r_sepc(void) { uint64_t x; asm volatile("csrr %0, sepc":"=r"(x));   return x; }
static inline uint64_t r_stval(void) { uint64_t x; asm volatile("csrr %0, stval":"=r"(x));  return x; }
static inline uint64_t r_satp(void) { uint64_t x; asm volatile("csrr %0, satp":"=r"(x));   return x; }
static inline uint64_t r_time(void) { uint64_t x; asm volatile("csrr %0, time":"=r"(x));   return x; }

static inline uint64_t r_sp(void) { uint64_t x; asm volatile("mv %0, sp":"=r"(x)); return x; }
static inline uint64_t r_fp(void) { uint64_t x; asm volatile("mv %0, s0":"=r"(x)); return x; }
static inline uint64_t r_tp(void) { uint64_t x; asm volatile("mv %0, tp":"=r"(x)); return x; }

// Clear SIE to stop further interrupts while panicking
static inline void panic_quiesce(void) {
	uint64_t s = r_sstatus();
	asm volatile("csrc sstatus, %0" :: "r"(1UL << 1) : "memory");
	(void)s;
}

static void backtrace(uint64_t fp, uint32_t max_frames) {
    for (uint32_t i = 0; i < max_frames && fp; i++) {
        uint64_t saved_ra = 0, prev_fp = 0;
        // Safely read memory; you can wrap these loads with simple bounds checks if you maintain kernel VA range.
        saved_ra = *(uint64_t*)(fp - 8);
        prev_fp = *(uint64_t*)(fp - 16);
        krprintf("  #%d: ra=%x fp=%x\n", i, saved_ra, fp);
        if (prev_fp == fp) break;
        fp = prev_fp;
    }
}

static void dump_stack(uint64_t sp) {
    uint64_t* p = (uint64_t*)(sp & ~0x7UL);
    for (byte i = 0; i < 16; i++) { // 16 * 8 = 128 bytes
        krprintf("  %x: %x\n", (uint64_t)(p + i), p[i]);
    }
}

static void dump_satp(uint64_t satp) {
    uint64_t mode = satp >> 60;
    uint64_t asid = (satp >> 44) & 0xFFFF;
    uint64_t ppn = satp & ((1UL << 44) - 1);
    const char* m = (mode == 8) ? "Sv39" : (mode == 9) ? "Sv48" : (mode == 10) ? "Sv57" : "bare/other";
    krprintf("satp: mode=%s asid=% ppn=%x\n", m, asid, ppn);
}

static const char* scause_str(uint64_t sc) {
    uint64_t intr = sc >> (sizeof(uint64_t) * 8 - 1);
    uint64_t code = sc & ((1UL << (sizeof(uint64_t) * 8 - 1)) - 1);
    if (intr) return "interrupt";
    switch (code) {
    case 0:  return "instruction address misaligned";
    case 1:  return "instruction access fault";
    case 2:  return "illegal instruction";
    case 5:  return "load access fault";
    case 7:  return "store/AMO access fault";
    case 8:  return "environment call from U";
    case 9:  return "environment call from S";
    case 12: return "instruction page fault";
    case 13: return "load page fault";
    case 15: return "store/AMO page fault";
    default: return "unknown";
    }
}

void panic(const char* format, ...) {
    //panic_quiesce();
	va_list ap; 
	va_start(ap, format);
	vkrprintf(format, ap);
	va_end(ap);

    /* TEMP, compiler won't let me comment these out without commenting out everything */
    if (0) {
        uint64_t ra = (uint64_t)__builtin_return_address(0);
        uint64_t sp = r_sp();
        uint64_t fp = r_fp();
        uint64_t tp = r_tp();
        uint64_t sstat = r_sstatus();
        uint64_t scau = r_scause();
        uint64_t sepc = r_sepc();
        uint64_t stv = r_stval();
        uint64_t satp = r_satp();
        //uint64_t tim = r_time();
        uint64_t tim = 0;

        krprintf("pc/ra=%x sp=%x fp=%x tp=%x\n", ra, sp, fp, tp);
        krprintf("sstatus=%x scause=%x (%s)\n", sstat, scau, scause_str(scau));
        krprintf("sepc=%x stval=%x time=%x\n", sepc, stv, tim);
        dump_satp(satp);

        krprintf("backtrace:\n");
        backtrace(fp, 32);

        krprintf("stack:\n");
        dump_stack(sp);
    }

    while (1) asm volatile("wfi");
}