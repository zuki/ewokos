# kernelを読む

## make

### kernel

```bash
machine/raspix/kernel/Makefile
    OS_IMG = kernel8.img, OS_ELF = kernel8.elf, QEMU_CMD = qemu-system-aarch64,
    ARCH_CFLAGS = "-Wa,-march=armv8-a", QEMU_FLAGS = "-M $(QEMU_MACHINE) -serial null -serial mon:stdio", MACHINE = raspix
    include machine/raspix/kernel/config.mk
        ARCH = aarch64, ARCH_VER = v8, LOAD_ADDRESS = 0x80000, QEMU_MACHINE = raspi3b, SMP = yes
    include machine/raspix/kernel/make.bsp
        $(ARCH_OBJS)
    include kernel/platform/${ARCH}/make.rule
        CC = aarch64-none-elf-gcc, SRC_DIR = kernel, ARCH_DIR = kernel/platform/aarch64/arch, BSP_DIR = bsp
```

### system

```bash
machine/raspix/system/Makefile
    HW = raspix
    BUILD_DIR = system/build/$(HW)
    TARGET_DIR = $(BUILD_DIR)/rootfs
    include system/platform/make.inc
        KERNEL_H = kernel/kernel/include/proto_t.h, syscalls.h, signals.h, interrupt.h, sysinfo.h, kevent.h, procinfo.h,
                 kenel/platform/aarch64/arch/common/include/arch_context.h, ewokos_config.h
        BUILD_DIR = system/build/raspix
        mkdir -p ${BUILD_DIR}/include
        mkdir -p ${BUILD_DIR}/lib
        mkdir -p ${BUILD_DIR}/rootfs
        cp -r $(KERNEL_H) $(BUILD_DIR)/include/
    basic_sys: kernel_heads
        rm -f $(BUILD_DIR)/lib/libbsp.a
        mkdir -p $(TARGET_DIR)/drivers/$(HW)
        cd libs; make
        cd ../../../system/basic; make
        cd drivers; make basic
        @cp -r etc/basic/* ${TARGET_DIR}/etc
            machine/raspix/system/etc/basic/init.rd
                @/bin/ipcserv /drivers/raspix/uartd /dev/tty0
                @set_stdio /dev/tty0                            // set_stdioはshellコマンド
                @/bin/ipcserv /drivers/timerd
                @/bin/ipcserv /drivers/ramfsd /tmp
                @/bin/ipcserv /drivers/nulld /dev/null
                @/bin/ipcserv /sbin/sessiond
                @/bin/bgrun /bin/session -r -t /dev/tty0
```

## グローバル/static変数

- kernel/kernel/hw_info.c
    ```c
    sys_info_t _sys_info;
    ```
- kernel/kernel/interrupt.c
    ```c
    static interrupt_item_t _interrupts[SYS_INT_MAX];
    ```
- kernel/kernel/irq.c
    ```c
    uint32_t _kernel_sec = 0;
    uint64_t _kernel_usec = 0;

    static uint64_t _last_usec = 0;
    static uint32_t _sec_tic = 0;
    ```
- kernel/kernel/kernel_config.c
    ```c
    kernel_conf_t _kernel_config;
    ```
- kernel/kernel/kernel.c
    ```c
    page_dir_entry_t* _kernel_vm = NULL;
    vsyscall_info_t* _kernel_vsyscall_info = NULL;
    ```
- kernel/kernel/kevqueue.c
    ```c
    static queue_t  _kev_queue;
    ```
- kernel/kernel/proc.c
    ```c
    bool _core_proc_ready = false;
    int32_t _core_proc_pid = -1;
    uint32_t _ipc_uid = 0;

    static proc_t **_task_table;
    static proc_vm_t *_proc_vm = NULL;
    static uint8_t  *_proc_vm_mark = NULL;
    static queue_t _ready_queue[CPU_MAX_CORES];
    static int32_t _current_proc[CPU_MAX_CORES];
    static uint32_t _use_core_id = 0;
    static uint32_t _proc_uuid = 0;
    static int32_t _last_create_pid = 0;
    ```
- kernel/kernel/semaphore.c
    ```c
    static semaphore_t _semaphores[SEMAPHORE_MAX];
    ```
- kernel/kernel/svc.c
    ```c
    static uint32_t _svc_counter[SYS_CALL_NUM];
    static uint32_t _svc_total;
    ```
- kernel/kernel/mm/dma.c
    ```c
    static uint32_t _dma_block_count = 0;
    static dma_block_t _dma_blocks[DMA_BLOCK_MAX];
    ```
- kernel/kernel/mm/kalloc.c
    ```c
    pages_ref_t _pages_ref;

    static __attribute__((__aligned__(PAGE_DIR_SIZE))) page_list_t *_free_list4k = 0;
    static __attribute__((__aligned__(1024))) page_list_t *_free_list1k = 0;
    ```
- kernel/kernel/mm/kmalloc_vm.c
    ```c
    static ewokos_addr_t km_vm_mem_tail = 0;
    static km_vm_t* _km_vm_head = NULL;
    static km_vm_t* _km_vm_tail = NULL;
    ```
- kernel/kernel/mm/kmalloc.c
    ```c
    static malloc_t _kmalloc;
    static ewokos_addr_t _kmalloc_mem_tail;
    ```
- kernel/lib/ext2/src/ext2read.c
    ```c
    static partition_t _partition;
    static partition_t _partitions[PARTITION_MAX];
    ```
- machine/raspix/kernel/bsp/hw_info.c
    ```c
    uint32_t _allocable_phy_mem_top = 0;
    uint32_t _allocable_phy_mem_base = 0;
    uint32_t _core_base_offset = 0;
    uint32_t _uart_type = UART_MINI;
    uint32_t _pi4 = 0;
    ```

## syscallの流れ

sysinfoプログラムを例とする

### クライアント側
1. system/basic/bin/sysinfo/sysinfo.c:
    ```c
    sys_info_t sys_info;
	syscall1(SYS_GET_SYS_INFO, (ewokos_addr_t)&sys_info);
    ```
2. system/build/include/ewoksys/syscall.h:
    ```c
        #define syscall1(code, arg0) syscall3_raw((code), (arg0), 0, 0)
        syscall3_raw(int, ewokos_addr_t, ewokos_addr_t, ewokos_addr_t);
    ```
3. system/basic/libc/libewoksys/ewoksys/src/syscall_aarch64.S:
    ```
    .global syscall3_raw
    syscall3_raw:
	    svc #0
	    ret
    ```

### サーバ側

1. kernel/platform/aarch64/arch/common/src/interrupt.S:
    ```
    interrupt_table_start: /*interrupt table, with syscall and irq items*/
    .balign 0x80	/*Synchronous*/
        #DEBUG #0x34
	    b sync_entry
    sync_entry:
        SAVE_IRQ_CONTEXT
        mrs x9, esr_el1
        and x9, x9, #0xFC000000
        mov x10, #0x54000000
        cmp x9, x10
        beq svc_entry
    svc_entry:
        mov x4, sp;
        bl svc_handler
    ```
2. kernel/kernel/src/svc.c:
    ```c
        svc_handler(code, arg0, arg1, arg2, context_t* ctx) {
            _svc_handler(code, arg0, arg1, arg2, ctx);
        }
        _svc_handler(code, arg0, arg1, arg2, context_t* ctx) {
            switch(code) {
            case SYS_GET_SYS_INFO:
		        sys_get_sys_info((sys_info_t*)arg0);
		        return;
            }
        }
        static void	sys_get_sys_info(sys_info_t* info) {
            memcpy(info, &_sys_info, sizeof(sys_info_t));
            info->max_proc_num = _kernel_config.max_proc_num;
            info->max_task_num = _kernel_config.max_task_num;
            info->max_task_per_proc = _kernel_config.max_task_per_proc;

            for(uint32_t i=0; i< _sys_info.cores; i++) {
                info->core_idles[i] = _cpu_cores[i].halt_proc->info.run_usec;
            }
        }
    ```

## libcの関数

- catプログラムを例とする

1. system/basic/bin/cat/caat.c
    ```c
    int main(int argc, char** argv) {
    	int fd = open(argv[1], 0);
    }
	```

2. system/basic/libc/libgloss/syscalls.c
    ```c
    int _open (const char * fname, int oflag, ...) {
        int fd = -1;
        bool created = false;
        fsinfo_t info;
        if (vfs_get_by_name(fname, &info) != 0) {}
        fd = vfs_open(&info, oflag);
        if (dev_open(info.mount_pid, fd, &info, oflag) != 0) {}
        return fd;
    }
    ```

3. system/basic/libc/libewoksys/ewoksys/src/vfs.c
    ```c
    int vfs_open(fsinfo_t* info, int oflag) {
        proto_t in, out;
        int res = ipc_call(get_vfsd_pid(), VFS_OPEN, &in, &out);
        return res;
    }
    // system/basic/libc/libewoksys/ewoksys/src/proc.c
    inline int get_vfsd_pid(void) {
        if(_vfsd_pid < 0)
            _vfsd_pid = ipc_serv_get(IPC_SERV_VFS);
        return _vfsd_pid;
    }
    ```

4. system/basic/libc/libwwoksys/ewoksys/src/ipc.c
    ```c
    inline int ipc_call(int to_pid, int call_id, const proto_t *ipkg, proto_t *opkg) {
        int ipc_id = 0;
        while (true) {
            ipc_id = syscall3(SYS_IPC_CALL, (ewokos_addr_t)to_pid, (ewokos_addr_t)call_id, (ewokos_addr_t)ipkg);
            break;
        }
        while (true) {
            res = syscall3(SYS_IPC_GET_RETURN, (ewokos_addr_t)to_pid, (ewokos_addr_t)ipc_id, (ewokos_addr_t)opkg);
            if(res == 0)
				break;
        }
        return res;
    }

    int ipc_serv_get(const char *ipc_serv_id) {
        int core_pid = syscall0(SYS_CORE_PID);
        if (ipc_call(core_pid, CORE_CMD_IPC_SERV_GET, &in, &out) == 0) {
            res = proto_read_int(&out);
        }
        return res;
    }
    ```

5. system/basic/libc/libwwoksys/ewoksys/src/devcmd.c
    ```c
    int dev_open(int dev_pid, int fd, fsinfo_t* info, int oflag) {
        proto_t in, out;
	    PF->init(&out);
	    PF->format(&in, "i,i,i", fd, info->node, oflag);
        int res = ipc_call(dev_pid, FS_CMD_OPEN, &in, &out);
        res =	proto_read_int(&out);
        if(res != 0)
            errno = proto_read_int(&out);
        else
            proto_read_to(&out, info, sizeof(fsinfo_t));
        return res;
    }
    ```