#ifndef MMU_H
#define MMU_H

#include <stdint.h>
#include "mmu_types.h"

#define MMU_OK                0
#define MMU_ERR_INVAL        -1
#define MMU_ERR_ALIGN        -2
#define MMU_ERR_VA_RANGE     -3
#define MMU_ERR_NO_L2_TABLE  -4
#define MMU_ERR_OVERLAP      -5

int mmu_init(const mmu_region_t *regions, uint32_t count);
void mmu_enable(void);
void mmu_disable(void);

void mmu_dcache_clean_range(uint64_t start, uint64_t size);

uint64_t mmu_read_sctlr(void);
uint64_t mmu_root_table_pa(void);
uint32_t mmu_used_l2_tables(void);

const mmu_region_t *boot_platform_get_mmu_regions(uint32_t *count);

#endif

