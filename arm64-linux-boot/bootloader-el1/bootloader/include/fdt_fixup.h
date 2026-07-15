#ifndef FDT_FIXUP_H
#define FDT_FIXUP_H

#include <stdint.h>

int fdt_fixup_runtime(void *src_fdt,
                      void *dst_fdt,
                      uint64_t dst_size,
                      uint64_t initrd_start,
                      uint64_t initrd_end,
                      const char *bootargs,
                      uint64_t *out_fdt_size);

#endif

