#include <stdint.h>
#include "uart.h"
#include "platform.h"
#include "mmu.h"
#include "image.h"
#include "fdt_fixup.h"
#include "initramfs_info.h"

int boot_main(void)
{
    const mmu_region_t *regions;
    uint32_t region_count;
    uint64_t fdt_size;
    uint64_t kernel_entry;
    uint64_t kernel_size;
    int rc;

    uart_init();

    uart_puts("boot: start\n");
    uart_puts("boot: hello from AArch64 bootloader\n");

    regions = boot_platform_get_mmu_regions(&region_count);

    rc = mmu_init(regions, region_count);
    if (rc != 0) {
        uart_puts("boot: mmu init failed, rc = 0x");
        uart_put_hex64((uint64_t)(uint32_t)rc);
        uart_puts("\n");
        while (1) {
        }
    }

    mmu_enable();

    uart_puts("boot: mmu enabled\n");
    uart_puts("boot: ttbr0       = 0x");
    uart_put_hex64(mmu_root_table_pa());
    uart_puts("\n");

    rc = image_validate(BOOT_IMAGE_ADDR, &kernel_entry, &kernel_size);
    if (rc != 0) {
        uart_puts("boot: kernel image validate failed, rc = 0x");
        uart_put_hex64((uint64_t)(uint32_t)rc);
        uart_puts("\n");
        while (1) {
        }
    }

    uart_puts("boot: kernel image crc ok\n");

    uart_puts("boot: image addr   = 0x");
    uart_put_hex64(BOOT_IMAGE_ADDR);
    uart_puts("\n");

    uart_puts("boot: kernel entry = 0x");
    uart_put_hex64(kernel_entry);
    uart_puts("\n");

    uart_puts("boot: kernel size  = 0x");
    uart_put_hex64(kernel_size);
    uart_puts("\n");

    uart_puts("boot: initrd start = 0x");
    uart_put_hex64(INITRD_LOAD_ADDR);
    uart_puts("\n");

    uart_puts("boot: initrd end   = 0x");
    uart_put_hex64((uint64_t)(INITRD_LOAD_ADDR + INITRAMFS_SIZE));
    uart_puts("\n");

    uart_puts("boot: fdt src addr = 0x");
    uart_put_hex64(FDT_LOAD_ADDR);
    uart_puts("\n");

    uart_puts("boot: fdt dst addr = 0x");
    uart_put_hex64(FDT_RUNTIME_ADDR);
    uart_puts("\n");

    rc = fdt_fixup_runtime((void *)FDT_LOAD_ADDR,
                           (void *)FDT_RUNTIME_ADDR,
                           FDT_RUNTIME_SIZE,
                           (uint64_t)INITRD_LOAD_ADDR,
                           (uint64_t)(INITRD_LOAD_ADDR + INITRAMFS_SIZE),
                           "console=ttyAMA0 earlycon=pl011,0x09000000 rdinit=/sbin/init",
                           &fdt_size);
    if (rc != 0) {
        uart_puts("boot: fdt runtime fixup failed, rc = 0x");
        uart_put_hex64((uint64_t)(uint32_t)rc);
        uart_puts("\n");
        while (1) {
        }
    }

    uart_puts("boot: fdt runtime fixup OK\n");
    uart_puts("boot: fdt size     = 0x");
    uart_put_hex64(fdt_size);
    uart_puts("\n");

    mmu_dcache_clean_range(FDT_RUNTIME_ADDR, fdt_size);
    mmu_disable();

    uart_puts("boot: jump kernel = 0x");
    uart_put_hex64(kernel_entry);
    uart_puts("\n");

    uart_puts("boot: x0(fdt) = 0x");
    uart_put_hex64(FDT_RUNTIME_ADDR);
    uart_puts("\n");

    __asm__ volatile(
        "mov x0, %0\n"
        "mov x1, xzr\n"
        "mov x2, xzr\n"
        "mov x3, xzr\n"
        "br  %1\n"
        :
        : "r"(FDT_RUNTIME_ADDR), "r"(kernel_entry)
        : "x0", "x1", "x2", "x3", "memory"
    );

    while (1) {
    }
}

