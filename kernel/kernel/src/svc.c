#include <kernel/kernel.h>
#include <kernel/interrupt.h>
#include <kernel/svc.h>
#include <kernel/schedule.h>
#include <kernel/system.h>
#include <kernel/proc.h>
#include <kernel/ipc.h>
#include <kernel/hw_info.h>
#include <kernel/semaphore.h>
#include <kernel/kevqueue.h>
#include <kernel/kconsole.h>
#include <kernel/signal.h>
#include <kernel/core.h>
#include <kernel/trace.h>
#include <mm/kalloc.h>
#include <mm/shm.h>
#include <mm/dma.h>
#include <mm/kmalloc.h>
#include <sysinfo.h>
#include <dev/uart.h>
#include <dev/timer.h>
#include <syscalls.h>
#include <kstring.h>
#include <kprintf.h>
#include <stddef.h>

static uint32_t _svc_counter[SYS_CALL_NUM];
static uint32_t _svc_total;

static void sys_kprint(const char* s, int32_t len) {
	(void)len;
	printf("%s", s);
}

static void sys_exit(context_t* ctx, int32_t res) {
	ctx->gpr[0] = 0;
	proc_t* cproc = get_current_proc();
	proc_exit(ctx, cproc, res);
}

static int32_t sys_signal_setup(uint32_t entry) {
	return proc_signal_setup(entry);
}

static void sys_signal(context_t* ctx, int32_t pid, int32_t sig) {
	ctx->gpr[0] = -1;
	proc_t* proc = proc_get(pid);
	proc_t* cproc = get_current_proc();
	if((cproc->info.uid > 0 &&
			cproc->info.uid != proc->info.uid) ||
			proc->info.uid < 0) {
		return;
	}
	proc_signal_send(ctx, proc, sig, true);
}

static void sys_signal_end(context_t* ctx) {
	proc_signal_end(ctx);
}

static int32_t sys_getpid(int32_t pid) {
	proc_t * cproc = get_current_proc();
	proc_t * proc = cproc;
	if(pid >= 0)
		proc = proc_get(pid);

	if(proc == NULL)
		return -1;

	if(cproc->info.uid > 0 && cproc->info.uid != proc->info.uid)
		return -1;

	proc_t* p = proc_get_proc(proc);
	if(p != NULL)
		return p->info.pid;
	return -1;
}

static int32_t sys_get_thread_id(void) {
	proc_t * cproc = get_current_proc();
	if(cproc == NULL)
		return -1;
	return cproc->info.pid; 
}

static void sys_usleep(context_t* ctx, uint32_t count) {
	proc_t * cproc = get_current_proc();
	ipc_task_t* ipc = proc_ipc_get_task(cproc);
	if(cproc->space->interrupt.state != INTR_STATE_IDLE)
		return;

	//no sleep longer than 100000 usec when handling interrupter/ipc task .
	if(ipc != NULL) {
		schedule(ctx);
		return;
	}

	proc_usleep(ctx, count);
}

static int32_t sys_malloc(int32_t size) {
	return (int32_t)proc_malloc(get_current_proc(), size);
}

static int32_t sys_msize(void) {
	return (int32_t)proc_msize(get_current_proc());
}

static void sys_free(int32_t p) {
	if(p == 0)
		return;
	proc_free(get_current_proc());
}

static void sys_fork(context_t* ctx) {
	proc_t *proc;
	proc = kfork(ctx, TASK_TYPE_PROC);
	if(proc == NULL) {
		ctx->gpr[0] = -1;
		return;
	}

	memcpy(&proc->ctx, ctx, sizeof(context_t));
	proc->ctx.gpr[0] = 0;
	ctx->gpr[0] = proc->info.pid;

	if(proc->info.state == CREATED && _core_proc_ready) {
		proc->info.state = BLOCK;
		proc->info.block_by = _core_proc_pid;
		proc->block_event = proc->info.pid;

		proc_t* cproc = get_current_proc();
		cproc->info.state = BLOCK;
		cproc->block_event = proc->info.pid;
		cproc->ctx.gpr[0] = proc->info.pid;
		cproc->info.block_by = _core_proc_pid;
		schedule(ctx);
	}
}

static void sys_detach(void) {
	proc_t* cproc = get_current_proc();
	cproc->info.father_pid = 0;
}

static void sys_thread(context_t* ctx, uint32_t entry, uint32_t func, int32_t arg) {
	ctx->gpr[0] = -1;
	proc_t *proc = kfork(ctx, TASK_TYPE_THREAD);
	if(proc == NULL)
		return;
	ctx->gpr[0] = proc->info.pid;

	proc->ctx.pc = entry;
	proc->ctx.lr = entry;
	proc->ctx.gpr[0] = func;
	proc->ctx.gpr[1] = arg;
}

static void sys_waitpid(context_t* ctx, int32_t pid) {
	proc_waitpid(ctx, pid);
}

static void sys_load_elf(context_t* ctx, const char* cmd, void* elf, uint32_t elf_size) {
	if(elf == NULL) {
		printf("Panic: load elf content is NULL!\n");
		ctx->gpr[0] = -1;
		return;
	}

	if(strlen(cmd) >= PROC_INFO_MAX_CMD_LEN) {
		printf("Panic: proc cmd line too long!\n");
		ctx->gpr[0] = -1;
		return;
	}

	proc_t* cproc = get_current_proc();
	strcpy(cproc->info.cmd, cmd);
	if(proc_load_elf(cproc, elf, elf_size) != 0) {
		ctx->gpr[0] = -1;
		return;
	}

	ctx->gpr[0] = 0;
	memcpy(ctx, &cproc->ctx, sizeof(context_t));
}

static int32_t sys_proc_set_uid(int32_t uid) {
	proc_t* cproc = get_current_proc();
	if(cproc->info.uid > 0)	
		return -1;
	cproc->info.uid = uid;
	return 0;
}

static int32_t sys_proc_set_gid(int32_t gid) {
	proc_t* cproc = get_current_proc();
	if(cproc->info.uid > 0)	
		return -1;
	cproc->info.gid = gid;
	return 0;
}

static int32_t sys_proc_get_cmd(int32_t pid, char* cmd, int32_t sz) {
	proc_t* proc = proc_get(pid);
	if(proc == NULL)
		return -1;
	sstrncpy(cmd, proc->info.cmd, sz);
	return 0;
}

static void sys_proc_set_cmd(const char* cmd) {
	proc_t* cproc = get_current_proc();
	sstrncpy(cproc->info.cmd, cmd, PROC_INFO_MAX_CMD_LEN-1);
}

static void	sys_get_sys_info(sys_info_t* info) {
	if(info == NULL)
		return;
	memcpy(info, &_sys_info, sizeof(sys_info_t));
	info->max_proc_num = _kernel_config.max_proc_num;
	info->max_task_num = _kernel_config.max_task_num;
	info->max_task_per_proc = _kernel_config.max_task_per_proc;

	for(uint32_t i=0; i< _sys_info.cores; i++) {
		info->core_idles[i] = _cpu_cores[i].halt_proc->info.run_usec;
	}
}

static void	sys_get_sys_state(sys_state_t* info) {
	if(info == NULL)
		return;

	info->mem.free = get_free_mem_size();
	info->mem.shared = shm_alloced_size();
	info->kernel_sec = _kernel_sec;
	info->svc_total = _svc_total;
	memcpy(info->svc_counter, _svc_counter, SYS_CALL_NUM*4);
}

static int32_t sys_shm_get(int32_t id, uint32_t size, int32_t flag) {
	return (int32_t)shm_get(id, size, flag);
}

static void* sys_shm_map(int32_t id) {
	proc_t* cproc = proc_get_proc(get_current_proc());
	return shm_proc_map(cproc, id);
}

static int32_t sys_shm_unmap(void* p) {
	proc_t* cproc = proc_get_proc(get_current_proc());
	return shm_proc_unmap(cproc, p);
}

/*
static int32_t sys_shm_ref(int32_t id) {
	proc_t* cproc = proc_get_proc(get_current_proc());
	return shm_proc_ref(cproc->info.pid, id);
}
*/
	
static uint32_t sys_dma_map(uint32_t size) {
	proc_t* cproc = proc_get_proc(get_current_proc());
	if(cproc->info.uid > 0)
		return 0;

	uint32_t paddr = dma_alloc(cproc->info.pid, size);
	if(paddr == 0)
		return 0;

	map_pages_size(cproc->space->vm, paddr, paddr, size, AP_RW_RW, PTE_ATTR_DEV);
	flush_tlb();
	return paddr;
}

static uint32_t sys_mem_map(uint32_t vaddr, uint32_t paddr, uint32_t size) {
	proc_t* cproc = proc_get_proc(get_current_proc());
	if(cproc->info.uid > 0)
		return 0;

	if(paddr == DMA_MAGIC)
		return sys_dma_map(size);

	/*allocatable memory can only mapped by kernel,
	userspace can map upper address such as MMIO/FRAMEBUFFER... */
	if(paddr > _allocable_phy_mem_base && paddr < _allocable_phy_mem_top)
		return 0;
	map_pages_size(cproc->space->vm, vaddr, paddr, size, AP_RW_RW, PTE_ATTR_DEV);	
	flush_tlb();
	return vaddr;
}

static void sys_ipc_setup(context_t* ctx, uint32_t entry, uint32_t extra_data, uint32_t flags) {
	//proc_t* cproc = get_current_proc();
	ctx->gpr[0] = proc_ipc_setup(ctx, entry, extra_data, flags);
}

static void sys_ipc_call(context_t* ctx, int32_t serv_pid, int32_t call_id, int32_t arg_shm_id) {
	ctx->gpr[0] = 0;

	proc_t* client_proc = get_current_proc();
	serv_pid = get_proc_pid(serv_pid);
	proc_t* serv_proc = proc_get(serv_pid);

	if(client_proc->info.pid == serv_pid) { //can't do self ipc
		//printf("ipc can't call self service (client: %d, server: %d, call: 0x%x\n", client_proc->info.pid, serv_pid, call_id);
		return;
	}

	if(serv_proc == NULL ||
			serv_proc->space->ipc_server.entry == 0) {//no ipc service setup
		//printf("ipc not ready (client: %d, server: %d, call: 0x%x\n", client_proc->info.pid, serv_pid, call_id);
		return;
	}

	if(serv_proc->space->ipc_server.disabled) {
		ctx->gpr[0] = -1; // blocked if server disabled, should retry
		proc_block_on(ctx, serv_pid, (uint32_t)&serv_proc->space->ipc_server.disabled);
		return;
	}

	if(serv_proc->space->interrupt.state != INTR_STATE_IDLE) {
		//if((call_id & IPC_NON_RETURN) == 0) {
			ctx->gpr[0] = -1; // blocked if proc is on interrupt task, should retry
			proc_block_on(ctx, serv_pid, (uint32_t)&serv_proc->space->interrupt);
			return;
		//}
		//call_id = call_id | IPC_LAZY; //not do task immediately
		//serv_proc->space->interrupt.saved_state.state = READY;
	}

	ipc_task_t* ipc = proc_ipc_req(serv_proc, client_proc, call_id, arg_shm_id);
	if(ipc == NULL) {
		ctx->gpr[0] = -1; 
		proc_block_on(ctx, client_proc->info.pid, (uint32_t)&client_proc->ipc_buffer_clean); 
		return;
	}
	shm_proc_unmap_by_id(client_proc, arg_shm_id, false);
	shm_set_owner(arg_shm_id, serv_pid);

	client_proc->ipc_res.state = IPC_BUSY;
	ctx->gpr[0] = ipc->uid;
	if(ipc != proc_ipc_get_task(serv_proc)) // buffered ipc
		return;
	proc_ipc_do_task(ctx, serv_proc, client_proc->info.core);
}

static void sys_ipc_get_return_size(context_t* ctx, int32_t pid, uint32_t uid) {
	ctx->gpr[0] = 0;
	proc_t* client_proc = get_current_proc();
	if(uid == 0 || client_proc == NULL) {
		ctx->gpr[0] = -2;
		return;
	}
	pid = get_proc_pid(pid);

	if(client_proc->ipc_res.state != IPC_RETURN) { //block retry for serv return
		proc_t* serv_proc = proc_get(pid);
		ipc_task_t* ipc = proc_ipc_get_task(serv_proc);
		if(ipc == NULL) {
			ctx->gpr[0] = -2;
			return;
		}

		if((ipc->call_id & IPC_NON_RETURN) == 0 || ipc->uid != uid) {
			ctx->gpr[0] = -1;
			proc_block_on(ctx, serv_proc->info.pid, (uint32_t)&client_proc->ipc_res);
			return;
		}
		return;
	}

	if(client_proc->ipc_res.uid != uid) {
		ctx->gpr[0] = -2;
		return;
	}

	ctx->gpr[0] = client_proc->ipc_res.data.size;
}

static void sys_ipc_get_return(context_t* ctx, int32_t pid, uint32_t uid, proto_t* data) {
	ctx->gpr[0] = 0;
	proc_t* client_proc = get_current_proc();
	if(uid == 0 || client_proc == NULL) {
		ctx->gpr[0] = -2;
		return;
	}
	pid = get_proc_pid(pid);

	if(client_proc->ipc_res.state != IPC_RETURN) { //block retry for serv return
		proc_t* serv_proc = proc_get(pid);
		ipc_task_t* ipc = proc_ipc_get_task(serv_proc);
		if(ipc == NULL || ipc->uid != uid) {
			ctx->gpr[0] = -2;
			return;
		}
		return;
	}

	if(client_proc->ipc_res.uid != uid) {
		ctx->gpr[0] = -2;
		return;
	}

	if(data != NULL && data->data != NULL && 
				data->size == client_proc->ipc_res.data.size) {
		memcpy(data->data, client_proc->ipc_res.data.data, data->size);
	}

	client_proc->ipc_res.uid = 0;
	client_proc->ipc_res.state = IPC_IDLE;
	proto_clear(&client_proc->ipc_res.data);
}

static int32_t sys_ipc_get_info(uint32_t uid, int32_t* ipc_info) {
	proc_t* serv_proc = proc_get_proc(get_current_proc());
	ipc_task_t* ipc = proc_ipc_get_task(serv_proc);
	if(uid == 0 ||
			ipc == NULL ||
			ipc->uid != uid ||
			ipc->state != IPC_BUSY ||
			serv_proc->space->ipc_server.entry == 0) {
		return -1;
	}

	//ipc_info[0] = get_proc_pid(ipc->client_pid);
	ipc_info[0] = ipc->client_pid;
	ipc_info[1] = ipc->call_id;
	ipc_info[2] = ipc->arg_shm_id;
	return 0;
}

static void sys_ipc_set_return(context_t* ctx, uint32_t uid, proto_t* data) {
	proc_t* serv_proc = proc_get_proc(get_current_proc());
	ipc_task_t* ipc = proc_ipc_get_task(serv_proc);
	if(uid == 0 ||
			ipc == NULL ||
			ipc->uid != uid ||
			serv_proc->space->ipc_server.entry == 0 ||
			ipc->state != IPC_BUSY ||
			(ipc->call_id & IPC_NON_RETURN) != 0) {
		return;
	}

	proc_t* client_proc = proc_get(ipc->client_pid);
	if(client_proc != NULL) {
		client_proc->ipc_res.state = IPC_RETURN;
		client_proc->ipc_res.uid = uid;
		if(data != NULL) {
			proto_copy(&client_proc->ipc_res.data, data->data, data->size);
		}
		proc_wakeup(serv_proc->info.pid, client_proc->info.pid, (uint32_t)&client_proc->ipc_res);
		proc_switch_multi_core(ctx, client_proc, serv_proc->info.core);
	}
}

static void sys_ipc_end(context_t* ctx) {
	proc_t* serv_proc = proc_get_proc(get_current_proc());
	ipc_task_t* ipc = proc_ipc_get_task(serv_proc);
	if(serv_proc == NULL ||
			serv_proc->space->ipc_server.entry == 0 ||
			ipc == NULL)
		return;

	proc_restore_state(ctx, serv_proc, &serv_proc->space->ipc_server.saved_state);
	if(serv_proc->info.state == READY || serv_proc->info.state == RUNNING)
	//if(serv_proc->info.state == READY)
		proc_ready(serv_proc);

	//wake up request proc to get return
	proc_ipc_close(serv_proc, ipc);
	proc_wakeup(serv_proc->info.pid, -1, (uint32_t)&serv_proc->space->ipc_server); 

	if(proc_ipc_fetch(serv_proc) != 0)  {//fetch next buffered ipc
		proc_save_state(serv_proc, &serv_proc->space->ipc_server.saved_state);
		serv_proc->space->ipc_server.do_switch = true;
		proc_ready(serv_proc);
		//proc_ipc_do_task(ctx, serv_proc, serv_proc->info.core);
	}
	//else
	schedule(ctx);
}

static int32_t sys_ipc_disable(void) {
	proc_t* cproc = proc_get_proc(get_current_proc());
	ipc_task_t* ipc = proc_ipc_get_task(cproc);
	if(ipc != NULL && ipc->state != IPC_IDLE)
		return -1;
	cproc->space->ipc_server.disabled = true;
	return 0;
}

static void sys_ipc_enable(void) {
	proc_t* cproc = proc_get_proc(get_current_proc());
	if(!cproc->space->ipc_server.disabled)
		return;

	cproc->space->ipc_server.disabled = false;
	proc_wakeup(cproc->info.pid, -1, (uint32_t)&cproc->space->ipc_server.disabled);
}

static int32_t sys_proc_ping(int32_t pid) {
	proc_t* proc = proc_get_proc(proc_get(pid));
	if(proc == NULL || !proc->space->ready_ping)
		return -1;
	return 0;
}

static void sys_proc_ready_ping(void) {
	proc_t* proc = proc_get_proc(get_current_proc());
	proc->space->ready_ping = true;
}

static void sys_get_kevent(context_t* ctx, kevent_t* kev) {
	ctx->gpr[0] = -1;
	if(kev_pop(kev) != 0) {
		proc_block_on(ctx, -1, (uint32_t)kev_init);
		return;
	}
	ctx->gpr[0] = 0;
}

static proc_block_event_t* get_block_evt(proc_t* proc, uint32_t event) {
	if(proc == NULL || event == 0)
		return NULL;

	for(int32_t i=0; i<BLOCK_EVT_MAX; i++) {
		proc_block_event_t* block_evt = &proc->space->block_events[i];
		if(block_evt->event == event)
			return block_evt;
	}
	return NULL;
}

static void set_block_evt(proc_t* proc, uint32_t event) {
	if(proc == NULL || event == 0)
		return;

	for(int32_t i=0; i<BLOCK_EVT_MAX; i++) {
		proc_block_event_t* block_evt = &proc->space->block_events[i];
		if(block_evt->event == 0) {
			block_evt->event = event;
			block_evt->refs = 0;
			break;
		}
	}
}

static void sys_proc_block(context_t* ctx, int32_t pid_by, uint32_t evt) {
	proc_t* proc_by = proc_get_proc(proc_get(pid_by));
	proc_t* cproc = proc_get_proc(get_current_proc());
	if(proc_by == NULL || cproc == NULL)
		return;

	if(proc_ipc_get_task(cproc) != NULL) {//don't block the proc when it's serving ipc
		schedule(ctx);
		return;
	}

	if(evt != 0 && proc_by->info.pid != _core_proc_pid) {
		proc_block_event_t* block_evt = get_block_evt(proc_by, evt);
		if(block_evt != NULL) {
			if(block_evt->refs > 0) {
				block_evt->refs--;
				return;
			}
			else
				block_evt->event = 0;
		}
	}	

	proc_block_on(ctx, proc_by->info.pid, evt);
}

static void sys_proc_wakeup(context_t* ctx, int32_t pid, uint32_t evt) {
	(void)ctx;
	proc_t* proc_by = proc_get_proc(get_current_proc());
	if(proc_by->info.pid != _core_proc_pid) {
		proc_block_event_t* block_evt = get_block_evt(proc_by, evt);
		if(block_evt != NULL)
			block_evt->refs++;
		else
			set_block_evt(proc_by, evt);
	}
	proc_wakeup(proc_by->info.pid, pid, evt);
}

static void sys_core_proc_ready(void) {
	proc_t* cproc = get_current_proc();
	if(cproc->info.uid > 0)
		return;
	_core_proc_ready = true;
	_core_proc_pid = cproc->info.pid;
}

static int32_t sys_core_proc_pid(void) {
	return _core_proc_pid;
}

static int32_t sys_get_kernel_tic(uint32_t* sec, uint32_t* hi, uint32_t* low) {
	if(sec != NULL)
		*sec = _kernel_sec;
	if(hi != NULL) 
		*hi = _kernel_usec >> 32;
	if(low != NULL)
		*low = _kernel_usec & 0xffffffff;
	return 0;
}

static int32_t sys_interrupt_setup(uint32_t interrupt, uint32_t entry, uint32_t data) {
	proc_t * cproc = get_current_proc();
	if(cproc->info.uid > 0)
		return -1;
	return interrupt_setup(cproc, interrupt, entry, data);
}

static void sys_interrupt_end(context_t* ctx) {
	interrupt_end(ctx);
}

static inline void sys_soft_int(context_t* ctx, int32_t to_pid, uint32_t entry, uint32_t data) {
	proc_t* proc = proc_get_proc(get_current_proc());
	if(proc->info.uid > 0)
		return;
	interrupt_soft_send(ctx, to_pid, entry, data);
}

static inline int32_t sys_proc_uuid(int32_t pid) {
	proc_t* proc = proc_get(pid);
	if(proc == NULL)
		return 0;
	return proc->info.uuid;
}

static inline void sys_schd_core_lock(void) {
	proc_t* cproc = get_current_proc();
	cproc->schd_core_lock_counter = SCHD_CORE_LOCK_LIMIT;
}

static inline void sys_schd_core_unlock(void) {
	proc_t* cproc = get_current_proc();
	cproc->schd_core_lock_counter = 0;
}

static inline void sys_set_timer_intr_usec(uint32_t usec) {
	proc_t* cproc = get_current_proc();
	if(cproc->info.uid > 0)
		return;
	_kernel_config.timer_intr_usec = usec;
}

static inline void sys_root(void) {
#ifdef KCONSOLE
	kconsole_close();
#endif
}

static int sys_get_trace(int arg0) {
#ifdef SCHD_TRACE
	return get_trace((trace_t*)arg0);
#endif
}

static int sys_get_trace_fps(void) {
#ifdef SCHD_TRACE
	return get_trace_fps();
#endif
}

static void sys_pause_trace(void) {
#ifdef SCHD_TRACE
	pause_trace();
#endif
}

static void sys_resume_trace(void) {
#ifdef SCHD_TRACE
	resume_trace();
#endif
}

static inline void _svc_handler(int32_t code, int32_t arg0, int32_t arg1, int32_t arg2, context_t* ctx) {
	_svc_total++;
	_svc_counter[code]++;

	switch(code) {
	case SYS_EXIT:
		sys_exit(ctx, arg0);
		return;
	case SYS_SIGNAL_SETUP:
		sys_signal_setup(arg0);
		return;
	case SYS_SIGNAL:
		sys_signal(ctx, arg0, arg1);
		return;
	case SYS_SIGNAL_END:
		sys_signal_end(ctx);
		return;
	case SYS_MALLOC_EXPAND:
		ctx->gpr[0] = sys_malloc(arg0);
		return;
	case SYS_MALLOC_SIZE:
		ctx->gpr[0] = sys_msize();
		return;
	case SYS_FREE:
		sys_free(arg0);
		return;
	case SYS_GET_PID:
		ctx->gpr[0] = sys_getpid(arg0);
		return;
	case SYS_GET_THREAD_ID:
		ctx->gpr[0] = sys_get_thread_id();
		return;
	case SYS_USLEEP:
		sys_usleep(ctx, (uint32_t)arg0);
		return;
	case SYS_EXEC_ELF:
		sys_load_elf(ctx, (const char*)arg0, (void*)arg1, (uint32_t)arg2);
		return;
	case SYS_FORK:
		sys_fork(ctx);
		return;
	case SYS_DETACH:
		sys_detach();
		return;
	case SYS_WAIT_PID:
		sys_waitpid(ctx, arg0);
		return;
	case SYS_YIELD: 
		schedule(ctx);
		return;
	case SYS_PROC_SET_UID: 
		ctx->gpr[0] = sys_proc_set_uid(arg0);
		return;
	case SYS_PROC_GET_UID: 
		ctx->gpr[0] = get_current_proc()->info.uid;
		return;
	case SYS_PROC_SET_GID: 
		ctx->gpr[0] = sys_proc_set_gid(arg0);
		return;
	case SYS_PROC_GET_GID: 
		ctx->gpr[0] = get_current_proc()->info.gid;
		return;
	case SYS_PROC_GET_CMD: 
		ctx->gpr[0] = sys_proc_get_cmd(arg0, (char*)arg1, arg2);
		return;
	case SYS_PROC_SET_CMD: 
		sys_proc_set_cmd((const char*)arg0);
		return;
	case SYS_GET_SYS_INFO:
		sys_get_sys_info((sys_info_t*)arg0);
		return;
	case SYS_GET_SYS_STATE:
		sys_get_sys_state((sys_state_t*)arg0);
		return;
	case SYS_GET_KERNEL_TIC:
		ctx->gpr[0] = sys_get_kernel_tic((uint32_t*)arg0, (uint32_t*)arg1, (uint32_t*)arg2);
		return;
	case SYS_GET_PROC: 
		ctx->gpr[0] = (int32_t)get_proc(arg0, (procinfo_t*)arg1);
		return;
	case SYS_GET_PROCS_NUM: 
		ctx->gpr[0] = (int32_t)get_procs_num();
		return;
	case SYS_GET_PROCS: 
		ctx->gpr[0] = (int32_t)get_procs(arg0, (procinfo_t*)arg1);
		return;
	case SYS_PROC_SHM_GET:
		ctx->gpr[0] = (int32_t)sys_shm_get(arg0, arg1, arg2);
		return;
	case SYS_PROC_SHM_MAP:
		ctx->gpr[0] = (int32_t)sys_shm_map(arg0);
		return;
	case SYS_PROC_SHM_UNMAP:
		ctx->gpr[0] = sys_shm_unmap((void*)arg0);
		return;
	case SYS_THREAD:
		sys_thread(ctx, (uint32_t)arg0, (uint32_t)arg1, arg2);
		return;
	case SYS_KPRINT:
		sys_kprint((const char*)arg0, arg1);
		return;
	case SYS_MEM_MAP:
		ctx->gpr[0] = sys_mem_map((uint32_t)arg0, (uint32_t)arg1, (uint32_t)arg2);
		return;
	case SYS_IPC_SETUP:
		sys_ipc_setup(ctx, arg0, arg1, arg2);
		return;
	case SYS_IPC_CALL:
		sys_ipc_call(ctx, arg0, arg1, arg2);
		return;
	case SYS_IPC_GET_RETURN_SIZE:
		sys_ipc_get_return_size(ctx, arg0, (uint32_t)arg1);
		return;
	case SYS_IPC_GET_RETURN:
		sys_ipc_get_return(ctx, arg0, (uint32_t)arg1, (proto_t*)arg2);
		return;
	case SYS_IPC_SET_RETURN:
		sys_ipc_set_return(ctx, (uint32_t)arg0, (proto_t*)arg1);
		return;
	case SYS_IPC_END:
		sys_ipc_end(ctx);
		return;
	case SYS_IPC_GET_ARG:
		ctx->gpr[0] = sys_ipc_get_info((uint32_t)arg0, (int32_t*)arg1);
		return;
	case SYS_IPC_PING:
		ctx->gpr[0] = sys_proc_ping(arg0);
		return;
	case SYS_IPC_READY:
		sys_proc_ready_ping();
		return;
	case SYS_GET_KEVENT:
		sys_get_kevent(ctx, (kevent_t*)arg0);
		return;
	case SYS_WAKEUP:
		sys_proc_wakeup(ctx, arg0, arg1);
		return;
	case SYS_BLOCK:
		sys_proc_block(ctx, arg0, arg1);
		return;
	case SYS_CORE_READY:
		sys_core_proc_ready();
		return;
	case SYS_CORE_PID:
		ctx->gpr[0] = sys_core_proc_pid();
		return;
	case SYS_IPC_DISABLE:
		ctx->gpr[0] = sys_ipc_disable();
		return;
	case SYS_IPC_ENABLE:
		sys_ipc_enable();
		return;
	case SYS_INTR_SETUP:
		ctx->gpr[0] = sys_interrupt_setup((uint32_t)arg0, (uint32_t)arg1, (uint32_t)arg2);
		return;
	case SYS_INTR_END:
		sys_interrupt_end(ctx);
		return;
	case SYS_SEMAPHORE_ALLOC:
		ctx->gpr[0] = semaphore_alloc();
		return;
	case SYS_SEMAPHORE_FREE:
		semaphore_free(arg0);
		return;
	case SYS_SEMAPHORE_ENTER:
		semaphore_enter(ctx, arg0);
		return;
	case SYS_SEMAPHORE_QUIT:
		ctx->gpr[0] = semaphore_quit(arg0);
		return;
	case SYS_SOFT_INT:
		sys_soft_int(ctx, arg0, arg1, arg2);
		return;
	case SYS_PROC_UUID:
		ctx->gpr[0] = sys_proc_uuid(arg0);
		return;
	case SYS_V2P:
		ctx->gpr[0] = V2P(arg0);
		return;
	case SYS_P2V:
		ctx->gpr[0] = P2V(arg0);
		return;
	case SYS_SCHD_CORE_LOCK:	
		sys_schd_core_lock();
		return;	
	case SYS_SCHD_CORE_UNLOCK:	
		sys_schd_core_unlock();
		return;	
	case SYS_SET_TIMER_INTR_USEC:	
		sys_set_timer_intr_usec(arg0);
		return;	
	case SYS_CLOSE_KCONSOLE:	
		sys_root();
		return;	
	case SYS_GET_TRACE:	
		ctx->gpr[0] = sys_get_trace(arg0);
		return;	
	case SYS_GET_TRACE_FPS:	
		ctx->gpr[0] = sys_get_trace_fps();
		return;	
	case SYS_PAUSE_TRACE:	
		sys_pause_trace();
		return;	
	case SYS_RESUME_TRACE:	
		sys_resume_trace();
		return;	
	}
}

inline void svc_handler(int32_t code, int32_t arg0, int32_t arg1, int32_t arg2, context_t* ctx) {
	__irq_disable();

	kernel_lock();
	_svc_handler(code, arg0, arg1, arg2, ctx);
	kernel_unlock();
}
