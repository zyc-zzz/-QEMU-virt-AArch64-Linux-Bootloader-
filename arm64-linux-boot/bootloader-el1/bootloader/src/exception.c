#include <stdint.h>
#include "exception.h"
#include "uart.h"

extern void exception_vectors(void);

static inline void write_vbar_el1(uint64_t value)
{
    __asm__ volatile("msr vbar_el1, %0" :: "r"(value));
}

static inline void isb(void)
{
    __asm__ volatile("isb" ::: "memory");
}

static const char *vector_name(uint64_t vector_id)
{
    switch (vector_id) {
    case 0: return "current EL with SP0 sync";
    case 1: return "current EL with SP0 irq";
    case 2: return "current EL with SP0 fiq";
    case 3: return "current EL with SP0 serr";
    case 4: return "current EL with SPx sync";
    case 5: return "current EL with SPx irq";
    case 6: return "current EL with SPx fiq";
    case 7: return "current EL with SPx serr";
    case 8: return "lower EL AArch64 sync";
    case 9: return "lower EL AArch64 irq";
    case 10: return "lower EL AArch64 fiq";
    case 11: return "lower EL AArch64 serr";
    case 12: return "lower EL AArch32 sync";
    case 13: return "lower EL AArch32 irq";
    case 14: return "lower EL AArch32 fiq";
    case 15: return "lower EL AArch32 serr";
    default: return "unknown";
    }
}

static uint32_t esr_ec(uint64_t esr)
{
    return (uint32_t)((esr >> 26) & 0x3FU);
}

static uint32_t esr_iss(uint64_t esr)
{
    return (uint32_t)(esr & 0x01FFFFFFU);
}

static uint32_t data_abort_wnr(uint64_t esr)
{
    return (esr_iss(esr) >> 6) & 0x1U;
}

static uint32_t data_abort_fsc(uint64_t esr)
{
    return esr_iss(esr) & 0x3FU;
}

static const char *esr_ec_name(uint32_t ec)
{
    switch (ec) {
    case 0x20: return "instruction abort from lower EL";
    case 0x21: return "instruction abort from current EL";
    case 0x24: return "data abort from lower EL";
    case 0x25: return "data abort from current EL";
    case 0x15: return "svc aarch64";
    case 0x16: return "hvc aarch64";
    case 0x17: return "smc aarch64";
    default:   return "other";
    }
}

static int is_data_abort_ec(uint32_t ec)
{
    return (ec == 0x24U || ec == 0x25U);
}

static int is_instruction_abort_ec(uint32_t ec)
{
    return (ec == 0x20U || ec == 0x21U);
}

static int fsc_level(uint32_t fsc)
{
    switch (fsc) {
    case 0x04: return 0;
    case 0x05: return 1;
    case 0x06: return 2;
    case 0x07: return 3;

    case 0x09: return 1;
    case 0x0A: return 2;
    case 0x0B: return 3;

    case 0x0C: return 0;
    case 0x0D: return 1;
    case 0x0E: return 2;
    case 0x0F: return 3;

    default: return -1;
    }
}

static const char *fsc_name(uint32_t fsc)
{
    switch (fsc) {
    case 0x00: return "address size fault level 0";
    case 0x01: return "address size fault level 1";
    case 0x02: return "address size fault level 2";
    case 0x03: return "address size fault level 3";

    case 0x04: return "translation fault level 0";
    case 0x05: return "translation fault level 1";
    case 0x06: return "translation fault level 2";
    case 0x07: return "translation fault level 3";

    case 0x09: return "access flag fault level 1";
    case 0x0A: return "access flag fault level 2";
    case 0x0B: return "access flag fault level 3";

    case 0x0C: return "permission fault level 0";
    case 0x0D: return "permission fault level 1";
    case 0x0E: return "permission fault level 2";
    case 0x0F: return "permission fault level 3";

    case 0x10: return "synchronous external abort";
    case 0x11: return "synchronous tag check fault";
    case 0x18: return "alignment fault";
    default:   return "other/unknown";
    }
}

void exception_init(void)
{
    write_vbar_el1((uint64_t)(uintptr_t)exception_vectors);
    isb();
}

void exception_handle(exception_frame_t *frame)
{
    uint32_t ec;
    uint32_t iss;
    uint32_t fsc;
    int level;

    uart_puts("\n");
    uart_puts("boot: exception occurred\n");

    uart_puts("boot: vector      = 0x");
    uart_put_hex64(frame->vector_id);
    uart_puts(" (");
    uart_puts(vector_name(frame->vector_id));
    uart_puts(")\n");

    ec = esr_ec(frame->esr_el1);
    iss = esr_iss(frame->esr_el1);

    uart_puts("boot: ESR_EL1     = 0x");
    uart_put_hex64(frame->esr_el1);
    uart_puts("\n");

    uart_puts("boot: ESR.EC      = 0x");
    uart_put_hex64((uint64_t)ec);
    uart_puts(" (");
    uart_puts(esr_ec_name(ec));
    uart_puts(")\n");

    uart_puts("boot: ESR.ISS     = 0x");
    uart_put_hex64((uint64_t)iss);
    uart_puts("\n");

    if (is_data_abort_ec(ec) || is_instruction_abort_ec(ec)) {
        fsc = data_abort_fsc(frame->esr_el1);
        level = fsc_level(fsc);

        uart_puts("boot: FSC         = 0x");
        uart_put_hex64((uint64_t)fsc);
        uart_puts(" (");
        uart_puts(fsc_name(fsc));
        uart_puts(")\n");

        if (is_data_abort_ec(ec)) {
            uart_puts("boot: WnR         = ");
            uart_puts(data_abort_wnr(frame->esr_el1) ? "write\n" : "read\n");
        }

        if (level >= 0) {
            uart_puts("boot: fault level = ");
            uart_put_hex64((uint64_t)level);
            uart_puts("\n");
        }
    }

    uart_puts("boot: ELR_EL1     = 0x");
    uart_put_hex64(frame->elr_el1);
    uart_puts("\n");

    uart_puts("boot: FAR_EL1     = 0x");
    uart_put_hex64(frame->far_el1);
    uart_puts("\n");

    uart_puts("boot: SPSR_EL1    = 0x");
    uart_put_hex64(frame->spsr_el1);
    uart_puts("\n");

    uart_puts("boot: CurrentEL   = 0x");
    uart_put_hex64(frame->current_el);
    uart_puts("\n");

    uart_puts("boot: SP          = 0x");
    uart_put_hex64(frame->sp);
    uart_puts("\n");

    uart_puts("boot: X0          = 0x");
    uart_put_hex64(frame->x0);
    uart_puts("\n");

    uart_puts("boot: X1          = 0x");
    uart_put_hex64(frame->x1);
    uart_puts("\n");

    uart_puts("boot: X29         = 0x");
    uart_put_hex64(frame->x29);
    uart_puts("\n");

    uart_puts("boot: X30         = 0x");
    uart_put_hex64(frame->x30);
    uart_puts("\n");

    uart_puts("boot: system halted in exception handler\n");

    while (1) {
    }
}

