# インストール

## brewによるツールのインストール

```sh
$ brew install arm-none-eabi-gcc
$ brew install lua
$ brew install e2tools
$ brew install --cask macfuse
$ brew install xmake
```

## カーネルビルド

### xmakeによる

```sh
$ git status
$ git status
On branch ja
Changes not staged for commit:
  (use "git add <file>..." to update what will be committed)
  (use "git restore <file>..." to discard changes in working directory)
	modified:   kernel/hardware/arm/arch/common/src/irq.c
	modified:   kernel/hardware/arm/arch/v7/neon.c
	modified:   kernel/hardware/arm/raspi/lib/bcm283x/src/pl011_uart.c
	modified:   kernel/hardware/arm/raspi/pix/bsp/core.c
	modified:   kernel/hardware/arm/raspi/pix/xmake.lua
	modified:   kernel/kernel/include/kernel/elf.h
	modified:   kernel/kernel/src/irq.c
	modified:   kernel/kernel/src/kernel.c
	modified:   kernel/kernel/src/kernel_config.c
	modified:   kernel/kernel/src/mm/shm.c
	modified:   kernel/kernel/src/mm/trunkmem.c
	modified:   kernel/kernel/src/proc.c
	modified:   kernel/kernel/src/svc.c
	modified:   kernel/lib/ext2/src/ext2read.c
	modified:   kernel/lib/include/kstring.h
	modified:   kernel/lib/sconf/src/sconf.c
	modified:   kernel/lib/src/mstr.c
	modified:   kernel/loadinit/sd/loadinit.c
	modified:   kernel/xmake.lua
	modified:   system/basic/Makefile
	modified:   system/full/drivers/consoled/xmake.lua
	modified:   system/full/drivers/displayd/xmake.lua
	modified:   system/full/drivers/fontd/xmake.lua
$ cd kernel
$ ls -a
.		dev		kernel		loadinit	xmake.lua
..		hardware	lib		makefile
$ make raspi2.3 > xmake.log 2> xmake_err.log
$ ls -a
.		build		kernel		makefile	xmake_err.log
..		dev		lib		xmake.log
.xmake		hardware	loadinit	xmake.lua
```

- linkディレクトリが設定されておらず、link時エラーが発生してカーネルは作成されず

### makeによる

```diff
$ git diff kernel/hardware/arm/make.rule 
diff --git a/kernel/hardware/arm/make.rule b/kernel/hardware/arm/make.rule
index f775581e3..fc812bbf3 100644
--- a/kernel/hardware/arm/make.rule
+++ b/kernel/hardware/arm/make.rule
@@ -50,6 +50,7 @@ CFLAGS += $(OPTM) \
        -I$(SRC_DIR)/dev/include \
        -I$(ARCH_DIR)/$(ARCH) \
        -I$(ARCH_DIR)/common/include \
+       -I$(SDKROOT)/usr/include \
        -marm $(ARCH_CFLAGS) \
        -Wstrict-prototypes \
        -pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
@@ -65,6 +66,7 @@ CFLAGS += $(OPTM) \
        -fno-builtin-strlen \
        -fno-builtin-strncpy \
        -fno-builtin-strncmp \
+       -Wno-builtin-declaration-mismatch \
        -std=c99

```

```sh
$ cd kernel/hardware/arm/raspi/pix
$ ls
Makefile		make.bsp		mkos.lds.qemu.S
README.md		make.bsp.clockwork	xmake.lua
bsp			make.bsp.pi4
config.mk		mkos.lds.S
$ make > make.log 2> make_err.log
$ ls
Makefile		kernel7.qemu.elf	make_err.log
README.md		kernel7.qemu.img	mkos.lds.S
bsp			make.bsp		mkos.lds.qemu.S
config.mk		make.bsp.clockwork	xmake.lua
kernel7.elf		make.bsp.pi4
kernel7.img		make.log
```

- カーネルイメージ`kernel7.img`が作成される
