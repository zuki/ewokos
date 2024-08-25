#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdint.h>
#include <stdbool.h>
#include <arch_context.h>

// 保存された状態
typedef struct {
    uint32_t  state;            // 状態
    int32_t   block_by;         // ブロックしたプロセス
    uint32_t  block_event;      // ブロックしたイベント
    uint32_t  sleep_counter;    // スリープする時間
    context_t ctx;              // コンテキスト情報
} saved_state_t;

#endif
