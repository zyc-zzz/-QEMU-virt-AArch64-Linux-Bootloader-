#include <stdint.h>
#include "fdt_fixup.h"
#include "libfdt.h"

static int ensure_chosen(void *fdt)
{
    int off = fdt_path_offset(fdt, "/chosen");
    if (off >= 0) {
        return off;
    }
    if (off != -FDT_ERR_NOTFOUND) {
        return off;
    }
    return fdt_add_subnode(fdt, 0, "chosen");
}

int fdt_fixup_runtime(void *src_fdt,
                      void *dst_fdt,
                      uint64_t dst_size,
                      uint64_t initrd_start,
                      uint64_t initrd_end,
                      const char *bootargs,
                      uint64_t *out_fdt_size)
{
    int rc;
    int chosen;

    if (src_fdt == 0 || dst_fdt == 0 || bootargs == 0 || dst_size == 0) {
        return -FDT_ERR_BADVALUE;
    }

    rc = fdt_open_into(src_fdt, dst_fdt, (int)dst_size);
    if (rc < 0) {
        return rc;
    }

    chosen = ensure_chosen(dst_fdt);
    if (chosen < 0) {
        return chosen;
    }

    rc = fdt_setprop_string(dst_fdt, chosen, "bootargs", bootargs);
    if (rc < 0) {
        return rc;
    }

    rc = fdt_setprop_u64(dst_fdt, chosen, "linux,initrd-start", initrd_start);
    if (rc < 0) {
        return rc;
    }

    rc = fdt_setprop_u64(dst_fdt, chosen, "linux,initrd-end", initrd_end);
    if (rc < 0) {
        return rc;
    }

    rc = fdt_pack(dst_fdt);
    if (rc < 0) {
        return rc;
    }

    if (out_fdt_size != 0) {
        *out_fdt_size = (uint64_t)fdt_totalsize(dst_fdt);
    }

    return 0;
}

