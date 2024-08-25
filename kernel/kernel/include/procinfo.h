#ifndef PROC_INFO_H
#define PROC_INFO_H

#include <stdint.h>
#include <stdbool.h>

#define PROC_INFO_MAX_CMD_LEN 256

// プロセスの状態
enum {
	UNUSED = 0,
	CREATED,
	SLEEPING,
	WAIT,
	BLOCK,
	READY,
	RUNNING,
	ZOMBIE
};

// タスク種別
enum {
	TASK_TYPE_PROC = 0,
	TASK_TYPE_THREAD,
	TASK_TYPE_VFORK
};

// IPCの状態
enum {
	IPC_IDLE = 0,
	IPC_BUSY,
	IPC_RETURN,
	IPC_DISABLED
};


#define IPC_DEFAULT          0x0
#define IPC_NON_BLOCK        0x01
#define IPC_NON_RETURN       0x80000000
#define IPC_LAZY             0x40000000
#define IPC_NON_RETURN_MASK  0x1fffffff

// プロセス情報構造体
typedef struct {
	uint32_t uuid;
	int32_t  type; 
	uint32_t core;
	int32_t  pid; 
	int32_t  father_pid;
	int32_t  uid; 
	int32_t  gid; 
	int32_t  state; 
	int32_t  block_by;
	int32_t  wait_for;
	uint32_t start_sec;
	uint32_t run_usec;
	uint32_t heap_size;
	uint32_t shm_size;
	char     cmd[PROC_INFO_MAX_CMD_LEN];
} procinfo_t;
	
#endif
