# raspi2のsys_info_t 値

```c
typedef struct {
	char        machine[32];                // "raspberry-pi2b"
	char        arch[16];                   // "armv7"
	uint32_t    phy_mem_size;               // 1GB
	uint32_t    phy_offset;                 // 0
	uint32_t    kernel_base;                // 0x8000_0000
	uint32_t    vector_base;                // 0

	mmio_info_t mmio;                       
	        uint32_t phy_base;              // 0x3f00_0000
	        uint32_t v_base;                // 0xe000_0000
	        uint32_t size;                  // 30MB
	dma_info_t  dma;
            uint32_t size;                  // 256KB
            uint32_t phy_base;              // 0x3bc0_0000 = 0x4000_0000 - 0x400_0000 - 0x0040_0000
	fb_info_t   fb;
            uint32_t phy_base;
            uint32_t v_base;
            uint32_t size;
	uint32_t    cores;                      // 4
	uint32_t    core_idles[MAX_CORE_NUM];

	uint32_t    max_proc_num;               // 64
	uint32_t    max_task_num;               // 256
	uint32_t    max_task_per_proc;          // 64
} sys_info_t;
```

# qemuメモリマップ

```sh
(qemu) info mtree
address-space: bcm2835-mbox-memory
  0000000000000000-000000000000008f (prio 0, i/o): bcm2835-mbox
    0000000000000010-000000000000001f (prio 0, i/o): bcm2835-fb
    0000000000000080-000000000000008f (prio 0, i/o): bcm2835-property

address-space: cpu-memory-0
address-space: cpu-memory-1
address-space: cpu-memory-2
address-space: cpu-memory-3
address-space: cpu-secure-memory-0
address-space: cpu-secure-memory-1
address-space: cpu-secure-memory-2
address-space: cpu-secure-memory-3
address-space: memory
  0000000000000000-ffffffffffffffff (prio 0, i/o): system
    0000000000000000-000000003fffffff (prio 0, ram): ram
    000000003f000000-000000003fffffff (prio 1, i/o): bcm2835-peripherals
      000000003f003000-000000003f00301f (prio 0, i/o): bcm2835-sys-timer
      000000003f004000-000000003f004fff (prio -1000, i/o): bcm2835-txp
      000000003f006000-000000003f006fff (prio 0, i/o): mphi
      000000003f007000-000000003f007fff (prio 0, i/o): bcm2835-dma
      000000003f00b200-000000003f00b3ff (prio 0, i/o): bcm2835-ic
      000000003f00b400-000000003f00b43f (prio -1000, i/o): bcm2835-sp804
      000000003f00b800-000000003f00bbff (prio 0, i/o): bcm2835-mbox
      000000003f100000-000000003f1001ff (prio 0, i/o): bcm2835-powermgt
      000000003f101000-000000003f102fff (prio 0, i/o): bcm2835-cprman
      000000003f104000-000000003f10400f (prio 0, i/o): bcm2835-rng
      000000003f200000-000000003f200fff (prio 0, i/o): bcm2835_gpio
      000000003f201000-000000003f201fff (prio 0, i/o): pl011
      000000003f202000-000000003f202fff (prio 0, i/o): bcm2835-sdhost
      000000003f203000-000000003f2030ff (prio -1000, i/o): bcm2835-i2s
      000000003f204000-000000003f204017 (prio 0, i/o): bcm2835-spi
      000000003f205000-000000003f205023 (prio 0, i/o): bcm2835-i2c
      000000003f20f000-000000003f20f07f (prio -1000, i/o): bcm2835-otp
      000000003f212000-000000003f212007 (prio 0, i/o): bcm2835-thermal
      000000003f214000-000000003f2140ff (prio -1000, i/o): bcm2835-spis
      000000003f215000-000000003f2150ff (prio 0, i/o): bcm2835-aux
      000000003f300000-000000003f3000ff (prio 0, i/o): sdhci
      000000003f600000-000000003f6000ff (prio -1000, i/o): bcm2835-smi
      000000003f804000-000000003f804023 (prio 0, i/o): bcm2835-i2c
      000000003f805000-000000003f805023 (prio 0, i/o): bcm2835-i2c
      000000003f900000-000000003f907fff (prio -1000, i/o): bcm2835-dbus
      000000003f910000-000000003f917fff (prio -1000, i/o): bcm2835-ave0
      000000003f980000-000000003f990fff (prio 0, i/o): dwc2
        000000003f980000-000000003f980fff (prio 0, i/o): dwc2-io
        000000003f981000-000000003f990fff (prio 0, i/o): dwc2-fifo
      000000003fc00000-000000003fc00fff (prio -1000, i/o): bcm2835-v3d
      000000003fe00000-000000003fe000ff (prio -1000, i/o): bcm2835-sdramc
      000000003fe05000-000000003fe050ff (prio 0, i/o): bcm2835-dma-chan15
    0000000040000000-00000000400000ff (prio 0, i/o): bcm2836-control

address-space: I/O
  0000000000000000-000000000000ffff (prio 0, i/o): io

address-space: bcm2835-dma-memory
address-space: bcm2835-fb-memory
address-space: bcm2835-property-memory
address-space: dwc2
  0000000000000000-00000000ffffffff (prio 0, i/o): bcm2835-gpu
    0000000000000000-000000003fffffff (prio 0, ram): alias bcm2835-gpu-ram-alias[*] @ram 0000000000000000-000000003fffffff
    0000000040000000-000000007fffffff (prio 0, ram): alias bcm2835-gpu-ram-alias[*] @ram 0000000000000000-000000003fffffff
    000000007e000000-000000007effffff (prio 1, i/o): alias bcm2835-peripherals @bcm2835-peripherals 0000000000000000-0000000000ffffff
    0000000080000000-00000000bfffffff (prio 0, ram): alias bcm2835-gpu-ram-alias[*] @ram 0000000000000000-000000003fffffff
    00000000c0000000-00000000ffffffff (prio 0, ram): alias bcm2835-gpu-ram-alias[*] @ram 0000000000000000-000000003fffffff

memory-region: ram
  0000000000000000-000000003fffffff (prio 0, ram): ram

memory-region: bcm2835-peripherals
  000000003f000000-000000003fffffff (prio 1, i/o): bcm2835-peripherals
    000000003f003000-000000003f00301f (prio 0, i/o): bcm2835-sys-timer
    000000003f004000-000000003f004fff (prio -1000, i/o): bcm2835-txp
    000000003f006000-000000003f006fff (prio 0, i/o): mphi
    000000003f007000-000000003f007fff (prio 0, i/o): bcm2835-dma
    000000003f00b200-000000003f00b3ff (prio 0, i/o): bcm2835-ic
    000000003f00b400-000000003f00b43f (prio -1000, i/o): bcm2835-sp804
    000000003f00b800-000000003f00bbff (prio 0, i/o): bcm2835-mbox
    000000003f100000-000000003f1001ff (prio 0, i/o): bcm2835-powermgt
    000000003f101000-000000003f102fff (prio 0, i/o): bcm2835-cprman
    000000003f104000-000000003f10400f (prio 0, i/o): bcm2835-rng
    000000003f200000-000000003f200fff (prio 0, i/o): bcm2835_gpio
    000000003f201000-000000003f201fff (prio 0, i/o): pl011
    000000003f202000-000000003f202fff (prio 0, i/o): bcm2835-sdhost
    000000003f203000-000000003f2030ff (prio -1000, i/o): bcm2835-i2s
    000000003f204000-000000003f204017 (prio 0, i/o): bcm2835-spi
    000000003f205000-000000003f205023 (prio 0, i/o): bcm2835-i2c
    000000003f20f000-000000003f20f07f (prio -1000, i/o): bcm2835-otp
    000000003f212000-000000003f212007 (prio 0, i/o): bcm2835-thermal
    000000003f214000-000000003f2140ff (prio -1000, i/o): bcm2835-spis
    000000003f215000-000000003f2150ff (prio 0, i/o): bcm2835-aux
    000000003f300000-000000003f3000ff (prio 0, i/o): sdhci
    000000003f600000-000000003f6000ff (prio -1000, i/o): bcm2835-smi
    000000003f804000-000000003f804023 (prio 0, i/o): bcm2835-i2c
    000000003f805000-000000003f805023 (prio 0, i/o): bcm2835-i2c
    000000003f900000-000000003f907fff (prio -1000, i/o): bcm2835-dbus
    000000003f910000-000000003f917fff (prio -1000, i/o): bcm2835-ave0
    000000003f980000-000000003f990fff (prio 0, i/o): dwc2
      000000003f980000-000000003f980fff (prio 0, i/o): dwc2-io
      000000003f981000-000000003f990fff (prio 0, i/o): dwc2-fifo
    000000003fc00000-000000003fc00fff (prio -1000, i/o): bcm2835-v3d
    000000003fe00000-000000003fe000ff (prio -1000, i/o): bcm2835-sdramc
    000000003fe05000-000000003fe050ff (prio 0, i/o): bcm2835-dma-chan15
```
