ROOT := /home/zyc/arm64-linux-boot
BOOTDIR := $(ROOT)/bootloader
BRDIR := $(ROOT)/buildroot
TOOLSDIR := $(ROOT)/tools

RUN_SCRIPT := $(TOOLSDIR)/run_bootloader_stage6.sh

.PHONY: all bootloader buildroot run clean

all: bootloader

bootloader:
	$(MAKE) -C $(BOOTDIR)

buildroot:
	$(MAKE) -C $(BRDIR)

run: bootloader
	$(RUN_SCRIPT)

clean:
	$(MAKE) -C $(BOOTDIR) clean

