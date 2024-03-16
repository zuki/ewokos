#ifndef KERNEL_H
#define KERNEL_H

#include <mm/mmu.h>

#define KERNEL_PROC_RUN_RECOUNT_SEC   2
#define SCHEDULE_FREQ_DEF     512 // usecs (timer/schedule)

extern uint32_t _kernel_sec;
extern uint64_t _kernel_usec;
extern char _kernel_start[];
extern char _kernel_end[];
extern char _kernel_sp[];

extern char _bss_start[];
extern char _bss_end[];

extern page_dir_entry_t* _kernel_vm;
extern void set_vm(page_dir_entry_t* vm);

#define MAX_PROC_NUM_DEF  128
#define MAX_TASK_NUM_DEF  1024
#define MAX_TASK_PER_PROC_DEF 128

typedef struct {
	uint32_t timer_freq;	
	uint32_t cores;
	uint32_t schedule_freq;
	uint32_t timer_intr_usec;
	uint32_t uart_baud;

	uint32_t kmalloc_size;
	uint32_t max_proc_num;
	uint32_t max_task_num;
	uint32_t max_task_per_proc;

	struct {
		uint32_t width;
		uint32_t height;
		uint32_t depth;
		uint32_t rotate;
	} fb;
} kernel_conf_t;

extern kernel_conf_t _kernel_config;
extern void load_kernel_config(void);

#endif
