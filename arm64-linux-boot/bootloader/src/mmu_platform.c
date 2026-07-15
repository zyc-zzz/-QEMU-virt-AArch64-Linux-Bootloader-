#include <stdint.h>
#include "platform.h"
#include "mmu.h"

#define SZ_2M   0x00200000ULL
#define SZ_1G   0x40000000ULL

#define RAM_BASE  0x40000000ULL
#define RAM_SIZE  0x40000000ULL

static const mmu_region_t virt_regions[] = {
    {
        .name  = "uart0",
        .va    = UART0_BASE,
        .pa    = UART0_BASE,
        .size  = SZ_2M,
        .attr  = MMU_MEM_DEVICE,
        .flags = 0,
    },
    {
        .name  = "ram",
        .va    = RAM_BASE,
        .pa    = RAM_BASE,
        .size  = RAM_SIZE,
        .attr  = MMU_MEM_NORMAL,
        .flags = MMU_REGION_EXEC,
    },
};

const mmu_region_t *boot_platform_get_mmu_regions(uint32_t *count)
{
    if (count != 0) {
        *count = (uint32_t)(sizeof(virt_regions) / sizeof(virt_regions[0]));
    }

    return virt_regions;
}

