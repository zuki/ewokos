#include <kernel/hw_info.h>
#include <kernel/kernel.h>
#include <mm/mmu.h>
#include <kstring.h>
#include <stdbool.h>
#include <bcm283x/board.h>

#ifdef KERNEL_SMP
#include <kernel/core.h>
#endif

sys_info_t _sys_info;
uint32_t _allocatable_phy_mem_top = 0;
uint32_t _allocatable_phy_mem_base = 0;
uint32_t _core_base_offset = 0;

#ifdef PI4
static bool isPi4B1G(uint32_t revision) {
	return (revision == 0xa03111);
}
#else
static bool isPi2B(uint32_t revision) {
	return (revision == 0xa01040 ||
			revision == 0xa01041 ||
			revision == 0xa21041 ||
			revision == 0xa02042 ||
			revision == 0xa22042);
}

static bool isPi3A(uint32_t revision) {
	return (revision == 0x9020e0);
}

static bool isPi2W(uint32_t revision) {
	return (revision == 0x902120);
}

static bool isPi3B(uint32_t revision) {
	return (revision == 0xa020a0 ||
			revision == 0xa02082 ||
			revision == 0xa020d3 ||
			revision == 0xa22082 ||
			revision == 0xa220a0 ||
			revision == 0xa02100 ||
			revision == 0xa32082 ||
			revision == 0xa52082 ||
			revision == 0xa22083);
}
#endif
	
void sys_info_init(void) {
	memset(&_sys_info, 0, sizeof(sys_info_t));
	uint32_t pix_revision = bcm283x_board();

#ifdef PI4
	strcpy(_sys_info.machine, "raspi4B1G");
	_sys_info.phy_mem_size = 1024*MB;
	_sys_info.mmio.phy_base = 0xfe000000;
	_core_base_offset =  0x01800000;
#else
	_core_base_offset =  0x01000000;
	if(isPi2B(pix_revision)) {
		strcpy(_sys_info.machine, "raspi2");
		_sys_info.phy_mem_size = 1024*MB;
		_sys_info.mmio.phy_base = 0x3f000000;
	}
	else if(isPi3A(pix_revision)) {
		strcpy(_sys_info.machine, "raspi3");
		_sys_info.phy_mem_size = 512*MB;
		_sys_info.mmio.phy_base = 0x3f000000;
	}
	else if(isPi2W(pix_revision)) {
		strcpy(_sys_info.machine, "raspi2w");
		_sys_info.phy_mem_size = 512*MB;
		_sys_info.mmio.phy_base = 0x3f000000;
	}
	else if(isPi3B(pix_revision)) {
		strcpy(_sys_info.machine, "raspi3");
		_sys_info.phy_mem_size = 1024*MB;
		_sys_info.mmio.phy_base = 0x3f000000;
	}
#endif

	_sys_info.phy_offset = 0;
	_sys_info.kernel_base = KERNEL_BASE;
	_sys_info.mmio.v_base = MMIO_BASE;
	_sys_info.mmio.size = 16*MB;

	_allocatable_phy_mem_base = V2P(ALLOCATABLE_MEMORY_START);
	_allocatable_phy_mem_top = _sys_info.phy_offset + _sys_info.phy_mem_size - 64*MB; //bcm283x framebuffer mem base
#ifdef KERNEL_SMP
	_sys_info.cores = get_cpu_cores();
#else
	_sys_info.cores = 1;
#endif
}

void arch_vm(page_dir_entry_t* vm) {
	uint32_t vbase = _sys_info.mmio.v_base + _core_base_offset;
	uint32_t pbase = _sys_info.mmio.phy_base + _core_base_offset;
	map_page(vm, vbase, pbase, AP_RW_D, 1);
	map_page(vm, pbase, pbase, AP_RW_D, 1);
}

