# インストール

## brewによるツールのインストール

```sh
$ brew install arm-none-eabi-gcc	# 後で変更
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

### toolchainを変える

```sh
$ brew uninstall arm-node-eabi-gcc
$ brew autoremove
$ brew install --cask gcc-arm-embedded
$ vi kernel/hardware/arm/make.rule
-I$(SDKROOT)/usr/include を削除
$ cd kernel/hardware/arm/raspi/pix
$ make
$ ls
Makefile		kernel7.qemu.elf	make_err.log
README.md		kernel7.qemu.img	mkos.lds.S
bsp			make.bsp		mkos.lds.qemu.S
config.mk		make.bsp.clockwork	xmake.lua
kernel7.elf		make.bsp.pi4
kernel7.img		make.log
```

## systemの作成

### basic systemのビルド

```sh
$ cd system/hardware/arm/raspix
$ make basic > basic.log 2> basic_err.log
$ ls ../../../build/raspix/
bin	drivers	etc	home	sbin
```

### rootfsの作成

```sh
$ make sd

====building ext2 format sdcard image====
mkdir -p build
dd if=/dev/zero of=build/root.ext2 bs=1k count=128k
131072+0 records in
131072+0 records out
134217728 bytes transferred in 0.260430 secs (515369689 bytes/sec)
mke2fs -b 1024 -I 128 build/root.ext2
mke2fs 1.47.1 (20-May-2024)
128-byte inodes cannot handle dates beyond 2038 and are deprecated
Creating filesystem with 131072 1k blocks and 32768 inodes
Filesystem UUID: 62b62f3c-7c2d-49bc-acaf-6f0f22b9bee4
Superblock backups stored on blocks: 
	8193, 24577, 40961, 57345, 73729

Allocating group tables: done                            
Writing inode tables: done                            
Writing superblocks and filesystem accounting information: done 

==== ext2 format sdcard image created  ====
$ $ ls -l build
total 262144
-rw-r--r--  1 zuki  staff  134217728  8 17 16:44 root.ext2
```

# QEMUでの実行

```sh
$ cd system/hardware/arm/raspix
$ make run

qemu-system-arm -M raspi2b -m 1024M -serial mon:stdio -kernel ../../../../kernel/hardware/arm/raspi/pix/kernel7.qemu.img -sd build/root.ext2
WARNING: Image format was not specified for 'build/root.ext2' and probing guessed raw.
         Automatically detecting the format is dangerous for raw images, write operations on block 0 will be restricted.
         Specify the 'raw' format explicitly to remove the restrictions.

=== ewokos booting ===

kernel: init kernel malloc     ... [OK]
kernel: init kernel event      ... [OK]
kernel: init sd                ... [OK]
kernel: load kernel config     ... [OK]

    machine              raspberry-pi2b
    arch                 armv7
    cores                4
    kernel_timer_freq    8192
    schedule_freq        512
    mem_offset           0x0
    mem_size             1024 MB
    kmalloc size         11 MB
    mmio_base            Phy:0x3f000000, V: 0xe0000000
    max proc num         64
    max task total       256
    max task per proc    64

kernel: remapping kernel mem   ... [OK]
kernel: init framebuffer       ... qemu-system-arm: warning: Blocked re-entrant IO on MemoryRegion: bcm2835-fb at addr: 0x0
[OK]
---------------------------------------------------
 ______           ______  _    _   ______  ______ 
(  ___ \|\     /|(  __  )| \  / \ (  __  )(  ___ \
| (__   | | _ | || |  | || (_/  / | |  | || (____
|  __)  | |( )| || |  | ||  _  (  | |  | |(____  )
| (___  | || || || |__| || ( \  \ | |__| |  ___) |
(______/(_______)(______)|_/  \_/ (______)\______)

kernel: init allocable memory  ... [ok] (934 MB)
kernel: init DMA               ... [OK]
kernel: init semaphore         ... [ok]
kernel: init irq               ... [ok]
kernel: init share memory      ... [ok]
kernel: init processes table   ... [ok] (64)
kernel: loading init process   ... [ok]
kernel: start core 1           ... [ok]
kernel: start core 2           ... [ok]
kernel: start core 3           ... [ok]
kernel: set timer              ... [ok]
kernel: start init process     ...
---------------------------------------------------

[init process started]
init: /sbin/core    [ok]
init: /sbin/vfsd    [ok]
init: /sbin/sdfsd    
    init sdc ... [ok]
    init ext2 fs ... [ok]
[ok]

init: loading '/etc/init0.rd' ... 
init: initailizing stdio at '/dev/tty0' ... [ok]

init: loading '/etc/init.rd' ... 
/bin/ipcserv /drivers/timerd               /dev/timer
/bin/ipcserv /drivers/nulld                /dev/null
/bin/ipcserv /drivers/ramfsd               /tmp
/bin/ipcserv /drivers/proc/sysinfod        /proc/sysinfo
/bin/ipcserv /drivers/proc/stated          /proc/state

+-----Ewok micro-kernel OS-----------------------+
| https://github.com/MisaZhu/EwokOS.git          |
+------------------------------------------------+
[/dev/tty0] login: root
[/dev/tty0] password: 
[/dev/tty0]:/# ls
drwx------  root     root        12288  [lost+found]
drwxr-xr-x  root     root         1024  [drivers]
drwxr-xr-x  root     root         1024  [bin]
drwxr-xr-x  root     root         1024  [sbin]
drwxr-xr-x  root     root         1024  [etc]
drwxr-xr-x  root     root         1024  [dev]
drwxrwxrwx  root     root         1024  [tmp]
drwxr-xr-x  root     root         1024  [proc]
[/dev/tty0]:/# ls drivers
drwxr-xr-x  root     root         1024  [proc]
-rwxr-xr-x  501      20         226772  netd
drwxr-xr-x  root     root         1024  [raspix]
-rwxr-xr-x  501      20         163064  timerd
-rwxr-xr-x  501      20         160020  nulld
-rwxr-xr-x  501      20         160284  ramfsd
[/dev/tty0]:/# ls bin
-rwxr-xr-x  501      20         138188  cat
-rwxr-xr-x  501      20         137336  mkfifo
-rwxr-xr-x  501      20         139452  svcinfo
-rwxr-xr-x  501      20         153096  md5
-rwxr-xr-x  501      20         207512  vi
-rwxr-xr-x  501      20         152904  dump
-rwxr-xr-x  501      20         138380  echo
-rwxr-xr-x  501      20         137956  ipcserv
-rwxr-xr-x  501      20         137304  pwd
-rwxr-xr-x  501      20         139152  test
-rwxr-xr-x  501      20         138868  whoami
-rwxr-xr-x  501      20         136976  sleep
-rwxr-xr-x  501      20         159560  shell
-rwxr-xr-x  501      20         138104  setux
-rwxr-xr-x  501      20         137816  uname
-rwxr-xr-x  501      20         142568  tsaver
-rwxr-xr-x  501      20         137716  elfinfo
-rwxr-xr-x  501      20         138024  grep
-rwxr-xr-x  501      20         138076  clear
-rwxr-xr-x  501      20         138132  kill
-rwxr-xr-x  501      20         161592  ps
-rwxr-xr-x  501      20         154860  json
-rwxr-xr-x  501      20         162824  devcmd
-rwxr-xr-x  501      20         137328  mkdir
-rwxr-xr-x  501      20         137280  mount
-rwxr-xr-x  501      20         153548  ls
-rwxr-xr-x  501      20         138828  cp
-rwxr-xr-x  501      20         139440  chown
-rwxr-xr-x  501      20         137936  chmod
-rwxr-xr-x  501      20         137560  rm
-rwxr-xr-x  501      20         140552  rx
-rwxr-xr-x  501      20         154736  login
-rwxr-xr-x  501      20         145872  session
[/dev/tty0]:/# ls sbin
-rwxr-xr-x  501      20         156072  sessiond
-rwxr-xr-x  501      20         183016  init
-rwxr-xr-x  501      20         143184  core
-rwxr-xr-x  501      20         163800  vfsd
-rwxr-xr-x  501      20         143148  telnetd
-rwxr-xr-x  501      20         157984  httpd
-rwxr-xr-x  501      20         191676  sdfsd
[/dev/tty0]:/# ls etc
-rw-r--r--  501      20            304  init.rd
-rw-r--r--  501      20             52  init0.rd
drwxr-xr-x  root     root         1024  [wlan]
drwxr-xr-x  root     root         1024  [kernel]
-rw-r--r--  501      20            175  passwd
[/dev/tty0]:/# cat etc/passwd
root:0:0:/home/root:/bin/shell:63a9f0ea7bb98050796b649e85481845
misa:100:100:/home/misa:/bin/shell:b201272a7344411b1dd09d7b8a3f25b3
guest:1000:1000:/tmp/home/guest:/bin/shell:[/dev/tty0]:/# 
[/dev/tty0]:/# cat etc/init.rd
/bin/ipcserv /drivers/timerd               /dev/timer
/bin/ipcserv /drivers/nulld                /dev/null
/bin/ipcserv /drivers/ramfsd               /tmp
/bin/ipcserv /drivers/proc/sysinfod        /proc/sysinfo
/bin/ipcserv /drivers/proc/stated          /proc/state

@/sbin/sessiond &
@/bin/session -r &[/dev/tty0]:/# ls /proc
-r--r--r--  root     root            0  sysinfo
-r--r--r--  root     root            0  state
[/dev/tty0]:/# cat /proc/sysinfo
machine: raspberry-pi2b
cores: 4
phy_mem_size: 1024 MB
max proc num: 64
max task total: 256
max task per proc: 64
max files per_proc: 128
mmio_base: 0x3f000000
[/dev/tty0]:/# exit
QEMU: Terminated
```

![実行画面](ewokos.png)
