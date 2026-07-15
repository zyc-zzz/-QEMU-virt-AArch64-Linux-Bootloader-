#include <stdint.h>
#include "platform.h"
#include "image.h"

static uint32_t crc32_update(uint32_t crc, uint8_t byte)
{
    crc ^= byte;

    for (int i = 0; i < 8; ++i) {
        if (crc & 1U) {
            crc = (crc >> 1) ^ 0xEDB88320U;
        } else {
            crc >>= 1;
        }
    }

    return crc;
}

uint32_t image_crc32_region(uint64_t addr, uint64_t size)
{
    const uint8_t *buf = (const uint8_t *)(uintptr_t)addr;
    uint32_t crc = 0xFFFFFFFFU;

    while (size--) {
        crc = crc32_update(crc, *buf++);
    }

    return crc ^ 0xFFFFFFFFU;
}

int image_parse_header(uint64_t image_addr,
                       uint64_t *kernel_entry,
                       uint64_t *kernel_size,
                       uint32_t *kernel_crc32)
{
    const boot_image_header_t *hdr = (const boot_image_header_t *)(uintptr_t)image_addr;

    if (hdr->magic != BOOT_IMAGE_MAGIC) {
        return -1;
    }

    if (hdr->header_size != IMAGE_HEADER_SIZE) {
        return -2;
    }

    if (hdr->kernel_entry != (image_addr + hdr->header_size)) {
        return -3;
    }

    if (hdr->kernel_entry != KERNEL_LOAD_ADDR) {
        return -4;
    }

    if (hdr->kernel_size == 0) {
        return -5;
    }

    *kernel_entry = hdr->kernel_entry;
    *kernel_size = hdr->kernel_size;
    *kernel_crc32 = hdr->kernel_crc32;
    return 0;
}

int image_validate(uint64_t image_addr, uint64_t *kernel_entry, uint64_t *kernel_size)
{
    uint32_t expected_crc;
    uint32_t actual_crc;
    int rc;

    rc = image_parse_header(image_addr, kernel_entry, kernel_size, &expected_crc);
    if (rc < 0) {
        return rc;
    }

    actual_crc = image_crc32_region(*kernel_entry, *kernel_size);
    if (actual_crc != expected_crc) {
        return -6;
    }

    return 0;
}
