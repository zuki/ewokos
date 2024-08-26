# EwokOS

- オリジナルREADMEを手元の環境似合わせて変更している

## 作者

	Misa.Z misa.zhu@gmail.com

## EwokOSについて

	オペレーティングシステム学習用のマイクロカーネルOS（arm32ビットとRISC-V32ビットに対応）。
	versatilepb / raspi1, 2, 3へのポータルは完成。raspi4は途中。

	- MMU対応
	- SMPマルチコア対応
	- コピー・オン・ライト機能
	- マルチプロセス
	- マルチスレッド
	- IPC
	- 仮想ファイルシステムサービス（全ては１つのファイル）
	- initrd用の非常にシンプルなramdisk
	- グラフィック用のフレームバッファデバイスサービス
	- UARTデバイスサービス
	- SDカード対応

## 環境とツール

- 開発にはMacbook Air M3を使用
- 実機はRaspberry Pi 3B+を使用
- uart経由でBasicシステムを稼働

```bash
$ brew install qemu
$ brew install --cask gcc-arm-embedded  # arm-none-eabi-gccではうまく動かない
$ brew install --cask macfuse
$ brew install minicom
$ brew install e2fsprogs        # mke2fs
$ brew install e2tools          # e2cp
$ brew install mtools           # mformat, mcopy
$ brew install util-linux       # sfdisk
```

## Minocomの設定

```bash
$ cat ~/.minirc.dfl
pu addcarreturn     Yes
pu port				/dev/cu.usbserial-AI057C9L
pu baudrate			19200
pu bits				8
pu parity			N
pu stopbits			1
pu rtscts			No
pu xonxoff			No
```

## カーネルのビルド

```bash
$ cd kernel/hardware/arm/raspi/pix
$ make > make.log 2> make_err.log
$ ls
Makefile		kernel7.qemu.elf	make_err.log
README.md		kernel7.qemu.img	mkos.lds.S
bsp			make.bsp		mkos.lds.qemu.S
config.mk		make.bsp.clockwork	xmake.lua
kernel7.elf		make.bsp.pi4
kernel7.img		make.log
```

## システムのビルド

```bash
$ cd system/hardware/arm/raspix
$ make basic
$ ls ../../../build/raspix/
bin	drivers	etc	home	sbin
$ ls build
root.ext2
```

## QEMUによる実行 (raspi2)

```bash
$ cd system/hardware/arm/raspix
$ make run			# username: root, password: root
```

## 実機での実行

```bash
$ cd system/hardware/arm/raspix
$ mkdir boot
$ cp bootcode.bin fixup.dat start.elf boot
$ vi boot/config.txt
enable_uart=1
$ cp kernel/hardware/arm/raspi/pix/kernel.img boot
$ mkdir sd
$ make -f make_sd.mk
$ ls
ewokos.img
# Raspberry Pi Imagerでewokos.imgをSDカードに書き込む
```

## xmakeビルドシステム

- xmakeによるビルドは設定が十分でないのか正常に動かなかった
- xmakeは使用しないことにした

