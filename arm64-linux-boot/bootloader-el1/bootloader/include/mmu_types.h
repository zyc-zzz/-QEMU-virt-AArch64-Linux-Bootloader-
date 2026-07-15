#ifndef MMU_TYPES_H
#define MMU_TYPES_H

#include <stdint.h>

typedef enum {
    MMU_MEM_DEVICE = 0,
    MMU_MEM_NORMAL = 1,
} mmu_mem_attr_t;

#define MMU_REGION_RO      (1U << 0)
#define MMU_REGION_EXEC    (1U << 1)

typedef struct {
    const char *name;
    uint64_t va;
    uint64_t pa;
    uint64_t size;
    mmu_mem_attr_t attr;
    uint32_t flags;
} mmu_region_t;

#endif

