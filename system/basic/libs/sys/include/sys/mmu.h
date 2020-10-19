#ifndef MMU_H
#define MMU_H

#include <sys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define get32(addr) (*((volatile uint32_t *)(addr)))
#define put32(addr, val) (*((volatile uint32_t *)(addr)) = (val))
#define get8(addr) (*((volatile uint8_t *)(addr)))
#define put8(addr, val) (*((volatile uint8_t *)(addr)) = (val))

extern uint32_t _mmio_base;
uint32_t mmio_map(bool cache);

#ifdef __cplusplus
}
#endif

#endif
