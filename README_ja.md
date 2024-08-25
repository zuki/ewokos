# EwokOS

## 作者

	Misa.Z misa.zhu@gmail.com

## WwokOSについて

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

### QEMU: 

qemuのファイル: hw/sd/sd.c を修正して再コンパイルする

```c
if(req.arg & ACMD41_ENQUIRY_MASK) { //modified by Misa.Z 
/*if (FIELD_EX32(sd->ocr & req.arg, OCR, VDD_VOLTAGE_WINDOW)) {*/
	/* We accept any voltage.  10000 V is nothing.
	*
	* Once we're powered up, we advance straight to ready state
	* unless it's an enquiry ACMD41 (bits 23:0 == 0).
	*/
	sd->state = sd_ready_state;
}
```

- これは不要
- `brew install qemu`
	
### risc-vのツールチェイン

https://github.com/riscv-software-src/homebrew-riscv

## MacOSXでの作業

### Homebrewによるツールのインストール

```sh
brew tap PX4/homebrew-px4               # これは使わない
brew install gcc-arm-none-eabi-49       # これは使わない 
brew install --cask gcc-arm-embedded    # 代わりにこれを使う
brew install e2tools
（インストール後にPATH環境変数を適切に設定する）
# USB/TTLドライバのダウンロード https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers
```
		
### ext2イメージの作成とマウント

#### macFUSEのインストール　[macFUSE校正機サイト](https://osxfuse.github.io/)

- 以下のビルドは行わない
- `brew install --cask macfuse`

##### ビルドツールのインストール

```sh
brew install e2fsprogs
brew install libtool 
brew install autoconf
brew install automake
```

##### [fuse-ext2](https://github.com/alperakcan/fuse-ext2)のインストール

```sh
wget https://github.com/alperakcan/fuse-ext2
./autogen.sh
CFLAGS="-idirafter/opt/gnu/include -idirafter/usr/local/include/osxfuse/ -idirafter/$(brew --prefix e2fsprogs)/include" LDFLAGS="-L/usr/local/opt/glib -L/usr/local/lib -L$(brew --prefix e2fsprogs)/lib" ./configure
sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
make
sudo make install
```

### ext2の作成とマウント

```sh
dd if=/dev/zero of=root.ext2 bs=1024 count=16384
mke2fs -b 1024 -I 128 root.ext2 	# block_size=1024 inode_size=128 でext2ファイルシステムを作成
mkdir -p tmp
fuse-ext2 -o force,rw+ img tmp
									# 必要なファイルをコピー
umount ./tmp
rm -r tmp
```

## ttyデバッグ

```sh	
install minicom (linux/mac)
```
	
## EwokOS カーネルイメージのビルド
	
```sh
cd kernel/hardware/arm/raspi/pix; make
```

## EwokOS rootfs (system/root.ext2) の作成
	
```sh
cd system/hardware/arm/raspix
make basic  # basic system
make full   # xgui
```
	
## QEMUによる実行 (raspi2)

```sh	
cd system/hardware/arm/raspix
make run			# EwokOSを実行 (username: root, password: root)
make debug			# デバッグサーバモードでEwokOSを実行
make gdb			# EwokOSをデバッグ（デバッグクライアントモード）
```

## xmakeビルドシステム

- xmakeによるビルドはまだxmakeの設定が十分でなく、正常に動かない

```sh
    "xmake":
        build kernel && system

    "xmake b xxx":
        only build package xxx
        package list:
           kernel
           system
           rootfs
           ...

    "xmake f -p xxx"
        switch platform to "xxx", defalut is miyoo
        platform list:
            miyoo
            raspi1
            raspi2.3
            raspi4
            
      "xmake c"
         clean project

      "xmake run qemu"
         run ewokos in qemu
         
      "xmake show"
         show current project infomation      
```

## コマンド
	
ほとんどのコマンドは`rootfs/bin`ディレクトリにある。たとえば、
	ls, ps, pwd, test など

## ソースコードリーディングガイド

チップ: アセンブリコードに深入りしないこと ;).

