CROSS_COMPILE ?= aarch64-linux-gnu-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy

LINUX_BUILD_DIR := $(firstword $(wildcard ../buildroot/output/build/linux-*))
ifeq ($(LINUX_BUILD_DIR),)
$(error Cannot find Buildroot Linux source under ../buildroot/output/build/linux-*)
endif

FDT_DIR := $(LINUX_BUILD_DIR)/scripts/dtc/libfdt
INITRAMFS_FILE := ../buildroot/output/images/rootfs.cpio
GEN_DIR := include/generated
GEN_HDR := $(GEN_DIR)/initramfs_info.h

CFLAGS := -Wall -Wextra -O2 \
-ffreestanding -fno-builtin -fno-stack-protector \
-nostdlib -nostartfiles -mcpu=cortex-a53 \
-Iinclude -I$(GEN_DIR) -I$(FDT_DIR)

LDFLAGS := -T linker.ld -nostdlib -nostartfiles -Wl,--build-id=none

OUT := out
TARGET := $(OUT)/bootloader.elf
TARGET_BIN := $(OUT)/bootloader.bin

OBJS := \
$(OUT)/start.o \
$(OUT)/boot.o \
$(OUT)/uart_pl011.o \
$(OUT)/string.o \
$(OUT)/mmu_core.o \
$(OUT)/mmu_platform.o \
$(OUT)/image.o \
$(OUT)/fdt_fixup.o \
$(OUT)/fdt.o \
$(OUT)/fdt_ro.o \
$(OUT)/fdt_rw.o \
$(OUT)/fdt_wip.o \
$(OUT)/fdt_sw.o \
$(OUT)/fdt_strerror.o

all: $(TARGET) $(TARGET_BIN)

$(OUT):
	mkdir -p $(OUT)

$(GEN_HDR): $(INITRAMFS_FILE)
	mkdir -p $(GEN_DIR)
	printf "#ifndef INITRAMFS_INFO_H\n#define INITRAMFS_INFO_H\n\n#define INITRAMFS_SIZE %sUL\n\n#endif\n" "$$(stat -c %s $(INITRAMFS_FILE))" > $(GEN_HDR)

$(OUT)/start.o: src/start.S | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/boot.o: src/boot.c $(GEN_HDR) | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/uart_pl011.o: src/uart_pl011.c | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/string.o: src/string.c | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/mmu_core.o: src/mmu_core.c | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/mmu_platform.o: src/mmu_platform.c | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/image.o: src/image.c | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/fdt_fixup.o: src/fdt_fixup.c $(GEN_HDR) | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/fdt.o: $(FDT_DIR)/fdt.c | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/fdt_ro.o: $(FDT_DIR)/fdt_ro.c | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/fdt_rw.o: $(FDT_DIR)/fdt_rw.c | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/fdt_wip.o: $(FDT_DIR)/fdt_wip.c | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/fdt_sw.o: $(FDT_DIR)/fdt_sw.c | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/fdt_strerror.o: $(FDT_DIR)/fdt_strerror.c | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(LD) $(OBJS) $(LDFLAGS) -o $@

$(TARGET_BIN): $(TARGET)
	$(OBJCOPY) -O binary $< $@

clean:
	rm -rf $(OUT)
	rm -rf $(GEN_DIR)

.PHONY: all clean

