BOOT_DIR := ../boot
IMAGE_DIR := sd
MACHINE_BUILD_DIR = build

SD_IMG := ewokos.img
BOOT_IMG := $(IMAGE_DIR)/boot.img
EXT2_IMG := $(IMAGE_DIR)/ext2.img
KERNEL_IMG := $(BOOT_DIR)/kernel8.img

MKE2FS := /opt/homebrew/Cellar/e2fsprogs/1.47.3/sbin/mke2fs
SFDISK := /opt/homebrew/Cellar/util-linux/2.41.1_1/sbin/sfdisk

# セクタサイズ: 0x200
SECTOR_SIZE := 512

# SD全体: 1 GiB = 1024 * 1024 * 1024 = 0x4000_000 (セクタ数単位 = 0x20_0000)
SD_SECTORS := 0x200000

# BOOTオフセット: 1MiB = 1024 * 1024 ; セクタ数: 0x10_0000 / 0x200 = 0x800 (セクタ数単位)
BOOT_OFFSET := 0x800
# BOOTセクタ = 256 MiB = 256 * 1024 * 1024 ; セクタ数: 0x1000_0000 / 0x200 = 0x8_0000
BOOT_SECTORS := 0x80000

# Ext2オフセット: 0x800 + 0x8_0000 = 0x8_0800
EXT2_OFFSET := $(shell echo $$(($(BOOT_OFFSET) + $(BOOT_SECTORS))))
# Exit2セクタ: 0x20_0000 - 0x80800 = 0x17f800 ; * 0x200 = 767 MiB
EXT2_SECTORS := $(shell echo $$(($(SD_SECTORS) - $(EXT2_OFFSET))))

.DELETE_ON_ERROR: $(BOOT_IMG) $(EXT2_IMG) $(SD_IMG)

all: sdcard

sdcard: $(SD_IMG)

$(KERNEL_IMG): ../kernel/kernel8.img
	cp ../kernel/kernel8.img $(BOOT_DIR)

$(BOOT_IMG): $(KERNEL_IMG) $(shell find $(BOOT_DIR) -type f)
	dd if=/dev/zero of=$@ seek=$$(($(BOOT_SECTORS) - 1)) bs=$(SECTOR_SIZE) count=1
# -F specify FAT32
# -c 1 specify one sector per cluster so that we can create a smaller one
	mformat -F -c 1 -i $@ ::
# Copy files into boot partition
	$(foreach x, $^, mcopy -i $@ $(x) ::$(notdir $(x));)

#$(EXT2_IMG): $(shell find $(TARGET_DIR) -type f)
$(EXT2_IMG):
	dd if=/dev/zero of=$@ bs=1024 count=767*1024
	$(MKE2FS) -b 1024 -t ext2 -I 128 $@
	cd $(MACHINE_BUILD_DIR); \
	sudo chown -R 0:0 . ;\
	sudo find . -type f | sudo e2cp -ap -G 0 -O 0 -d ../$@:/
#$(foreach x, $^, e2cp -ap -G0 -O0 -d $@:/ $(x);)


$(SD_IMG): $(BOOT_IMG) $(EXT2_IMG)
	dd if=/dev/zero of=$@ seek=$$(($(SD_SECTORS) - 1)) bs=$(SECTOR_SIZE) count=1
	printf "                                                                \
	  $(BOOT_OFFSET), $$(($(BOOT_SECTORS)*$(SECTOR_SIZE) / 1024))K, c,\n      \
	  $(EXT2_OFFSET), $$(($(EXT2_SECTORS)*$(SECTOR_SIZE) / 1024))K, L,\n  \
	" | $(SFDISK) $@
	dd if=$(BOOT_IMG) of=$@ seek=$(BOOT_OFFSET) conv=notrunc
	dd if=$(EXT2_IMG) of=$@ seek=$(EXT2_OFFSET) conv=notrunc

clean:
	rm -f $(BOOT_IMG)
	rm -f $(EXT2_IMG)
	rm -f $(SD_IMG)