BUILD_DIR := sd
TARGET_DIR := ../../../build/raspix
BOOT_DIR := boot

SD_IMG := ewokos.img
BOOT_IMG := $(BUILD_DIR)/boot.img
EXT2_IMG := $(BUILD_DIR)/ext2.img

# セクタサイズ: 0x200
SECTOR_SIZE := 512

# SD全体: 4 GiB = 512 * 4 * 2 * 1024 * 1024 (セクタ数単位 = 0x80_0000)
SD_SECTORS := 8*1024*1024

# BOOTオフセット: 1MiB = 512 * 2 * 1024 (セクタ数単位)
BOOT_OFFSET := 2048
# BOOTセクタ = 256MiB = 512 * 2 * 256 * 1024
BOOT_SECTORS := 512*1024

# Ext2オフセット: 
EXT2_OFFSET := $(shell echo $$(($(BOOT_OFFSET) + $(BOOT_SECTORS))))
# Exit2セクタ: 3836 MiB = 512 * 2 * 3836 * 1024
EXT2_SECTORS := $(shell echo $$(($(SD_SECTORS) - $(EXT2_OFFSET))))

.DELETE_ON_ERROR: $(BOOT_IMG) $(EXT2_IMG) $(SD_IMG)

all: $(SD_IMG)

$(BOOT_IMG): $(shell find $(BOOT_DIR) -type f)
	dd if=/dev/zero of=$@ seek=$$(($(BOOT_SECTORS) - 1)) bs=$(SECTOR_SIZE) count=1
# -F specify FAT32
# -c 1 specify one sector per cluster so that we can create a smaller one
	mformat -F -c 1 -i $@ ::
# Copy files into boot partition
	$(foreach x, $^, mcopy -i $@ $(x) ::$(notdir $(x));)

#$(EXT2_IMG): $(shell find $(TARGET_DIR) -type f)
$(EXT2_IMG):
	dd if=/dev/zero of=$@ bs=1024 count=3836*1024
	mke2fs -b 1024 -t ext2 -I 128 $@
	cd $(TARGET_DIR); \
	sudo chown -R 0:0 . ;\
	sudo find . -type f | sudo e2cp -ap -G 0 -O 0 -d ../../hardware/arm/raspix/$@:/
#$(foreach x, $^, e2cp -ap -G0 -O0 -d $@:/ $(x);)


$(SD_IMG): $(BOOT_IMG) $(EXT2_IMG)
	dd if=/dev/zero of=$@ seek=$$(($(SD_SECTORS) - 1)) bs=$(SECTOR_SIZE) count=1
	printf "                                                                \
	  $(BOOT_OFFSET), $$(($(BOOT_SECTORS)*$(SECTOR_SIZE) / 1024))K, c,\n      \
	  $(EXT2_OFFSET), $$(($(EXT2_SECTORS)*$(SECTOR_SIZE) / 1024))K, L,\n  \
	" | sfdisk $@
	dd if=$(BOOT_IMG) of=$@ seek=$(BOOT_OFFSET) conv=notrunc
	dd if=$(EXT2_IMG) of=$@ seek=$(EXT2_OFFSET) conv=notrunc

clean:
	rm -f $(SD_IMG) $(BOOT_IMG) $(EXT2_IMG)
