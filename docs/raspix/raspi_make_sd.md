1.download raspbian lite img file from https://www.raspberrypi.org/downloads/raspbian/

2.restore the img to a microsd card(size >= 4G)

3.format the ext4 partition with ext2 format by this command:

    sudo mke2fs -L rootfs -b 1024 -I 128 /dev/(SD_PARTITION_EXT4)

4.cd system and make (or make full)

5.copy system/build/rootfs/* to ext2 partition root dir

6.remove  kernel*.img files from boot partition root dir.

7.cd build path and remake kernel (with make clean)

8.copy kernel image file(kernel.img / kernel7.img) to boot partition

1. `Raspberry Pi Imager`でraspi3B+用の32Bit版LinuxをSDに書き込む

2. SDデバイスのデバイスを調べる

```sh
$ diskutil list
/dev/disk6 (external, physical):
   #:                       TYPE NAME                    SIZE       IDENTIFIER
   0:     FDisk_partition_scheme                        *15.6 GB    disk6
   1:             Windows_FAT_32 bootfs                  536.9 MB   disk6s1
   2:                      Linux                         5.0 GB     disk6s2
                    (free space)                         10.0 GB    -
```

3. Linuxパーティションをext2で初期化

```sh
$ sudo mke2fs -L rootfs -b 1024 -I 128 /dev/disk6s2
Password:
mke2fs 1.47.1 (20-May-2024)
/dev/disk6s2 contains a ext4 file system labelled 'rootfs'
	last mounted on Thu Jul  4 09:19:58 2024
Proceed anyway? (y,N) y
128-byte inodes cannot handle dates beyond 2038 and are deprecated
Creating filesystem with 4915200 1k blocks and 307200 inodes
Filesystem UUID: 840b51dc-6956-42df-aef7-167ee3d1b66b
Superblock backups stored on blocks: 
	8193, 24577, 40961, 57345, 73729, 204801, 221185, 401409, 663553, 
	1024001, 1990657, 2809857

Allocating group tables: done                            
Writing inode tables: done                            
Writing superblocks and filesystem accounting information: done
```

4. rootfsのコピー

- これは動くが非常に遅い。1時間ではまったく終わらない。

```sh
$ find . -type f | sudo e2cp -ap -G0 -O0 -d /dev/disk6s2:/
```
