#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>

#define BOOT_IMAGE_MAGIC 0x42494D47U

typedef struct {
    uint32_t magic;
    uint32_t header_size;
    uint64_t kernel_entry;
    uint64_t kernel_size;
    uint32_t kernel_crc32;
    uint32_t reserved;
} boot_image_header_t;

int image_parse_header(uint64_t image_addr,
                       uint64_t *kernel_entry,
                       uint64_t *kernel_size,
                       uint32_t *kernel_crc32);

uint32_t image_crc32_region(uint64_t addr, uint64_t size);

int image_validate(uint64_t image_addr, uint64_t *kernel_entry, uint64_t *kernel_size);

#endif
