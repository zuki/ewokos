#ifndef MMU_ARCH_H
#define MMU_ARCH_H

#include "mmudef.h"

#define PAGE_DIR_INDEX(x) ((uint32_t)x >> 20)
#define PAGE_INDEX(x) (((uint32_t)x >> 12) & 255)

#define PAGE_TABLE_TO_BASE(x) ((uint32_t)x >> 10)
#define BASE_TO_PAGE_TABLE(x) ((void *) ((uint32_t)x << 10))
#define PAGE_TO_BASE(x) ((uint32_t)x >> 12)

/* a 32-bit entry in hardware's PageDir table */
typedef struct {
	uint32_t type   : 2; // 0: fault, 0x1: page table, 0x2: section
	uint32_t sbz    : 3; //should be zero
	uint32_t domain : 4;
	uint32_t p      : 1; //ECC Enable, ignored by ARM1176JZF-S
	uint32_t base   : 22;	// 第2レベルの戦闘アドレスを1Kバイトアラインで設定
} page_dir_entry_t;

/* a 32-bit entry in hardware's page table */
typedef struct {
	uint32_t type       : 2; //0: fault, 0x1: large size(64k), 0x2: small size(4k), 0x3: small size with no-excute(4k)
	uint32_t writeback  : 1; //B 1: ライトバック、0: ライトスルー
	uint32_t cacheable  : 1; //C 1: キャッシュ可, 0: キャッシュ不可
	uint32_t ap         : 2; //Access Permissions,
                             //0x1: super-RW,user-NONE
                             //0x2: super-RW,user-R
                             //0x3: super-RW,user-RW
	uint32_t tex        : 3; //Type Extension Field, access control work with C,B
	uint32_t apx        : 1; //Access Permissions Extension Bit
	uint32_t sharable   : 1; // 1: ノーマルメモリ共有可, 0: ノーマルメモリ共有不可
	uint32_t ng         : 1; //Not-Global, 0 for global, 1 for local
	uint32_t base       : 20;	// 物理ページのアドレスを4Kバイトアラインで設定
} page_table_entry_t; 

typedef struct {
	uint32_t type       : 2; //0: fault, 0x1: large size(64k), 0x2: small size(4k), 0x3: small size with no-excute(4k)
	uint32_t writeback  : 1; //B
	uint32_t cacheable  : 1; //C
	uint32_t ap0        : 2; //Access Permissions,
                           //0x1: super-RW,user-NONE
                           //0x2 super-RW,user-R
                           //0x3 super-RW,user-RW
	uint32_t ap1 		: 2;
	uint32_t ap2 		: 2;
	uint32_t ap3 		: 2;
	uint32_t base       : 20;
} page_table_entry_v5_t; 

void set_pte_flags(page_table_entry_t* pte, uint32_t pte_attr);

#define PTE_ATTR_WRBACK          0
#define PTE_ATTR_DEV             1
#define PTE_ATTR_WRTHR           2
#define PTE_ATTR_WRBACK_ALLOCATE 3
#define PTE_ATTR_STRONG_ORDER    4
#define PTE_ATTR_NOCACHE         5

int32_t  map_page(page_dir_entry_t *vm, 
	ewokos_addr_t virtual_addr, 
	ewokos_addr_t physical,
	uint32_t access_permissions, 
	uint32_t pte_attr);

void unmap_page(page_dir_entry_t *vm, ewokos_addr_t virtual_addr);

ewokos_addr_t resolve_phy_address(page_dir_entry_t *vm, ewokos_addr_t virtual);
page_table_entry_t* get_page_table_entry(page_dir_entry_t *vm, ewokos_addr_t virtual);
void free_page_tables(page_dir_entry_t *vm);

void __set_translation_table_base(ewokos_addr_t);
void __flush_tlb(void);

#endif
