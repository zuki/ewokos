#include <kernel/irq.h>
#include <kernel/kernel.h>
#include <kernel/hw_info.h>
#include "../timer_arch.h"

// Core0 timers Interrupt control
#define CORE0_IRQ_CNTL_OFFSET    0x40
// Core0 IRQ Source
#define CORE0_IRQ_SOURCE_OFFSET  0x60

static void routing_core0_irq(void) {
  // _sys_info.mmio.v_base + _core_base_offset = 0x4000_0000
  uint32_t vbase = _sys_info.mmio.v_base + _core_base_offset + CORE0_IRQ_CNTL_OFFSET;
  // nCNTVIRQを有効に
  put32(vbase, 0x08);
}

// Core 0 保留割り込みソースの読み込み
static uint32_t read_core0_pending(void) {
  uint32_t vbase = _sys_info.mmio.v_base + _core_base_offset + CORE0_IRQ_SOURCE_OFFSET;
  return get32(vbase);
}

// arch固有の割り込み初期化
void irq_arch_init(void) {
	routing_core0_irq();
}

// 保有割り込みソースを取得
inline uint32_t irq_get(void) {
	uint32_t ret = 0;
	uint32_t pending = read_core0_pending();

    // タイマー割り込みの場合は再セット
	if (pending & 0x08 ) {
		ret = IRQ_TIMER0;       // bit 12を使用
		write_cntv_tval(_timer_tval); 
	}
	return ret;
}

// 割り込みを有効化（何もしない）
inline void irq_enable(uint32_t irq) {
	(void)irq;
}

// 割り込みを無効化（何もしない）
void irq_disable(uint32_t irq) {
	(void)irq;
}
