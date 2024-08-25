#ifndef ARCH_CONTEXT_H
#define ARCH_CONTEXT_H

#include <stdint.h>
#include <stdbool.h>

// コンテキスト構造体
typedef struct {
  uint32_t cpsr, pc, gpr[ 13 ], sp, lr;
} context_t;


#define CONTEXT_INIT(x) (x.cpsr = 0x50)

#endif
