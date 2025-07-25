# システムが立ち上がるまで

1. kernel/platform/aarch64/arch/v8/boot.S#__entry
2. machine/raspix/kernel/bsp/start.c#_boot_start()
3. kernel/kernel/src/kernel.c#_kernel_entry_c()

    1. 割り込みを無効に: __irq_disable();
    2. bss領域を初期化: memset(_bss_start, 0, (uint32_t)_bss_end - (uint32_t)_bss_start);
    3. システム情報を初期化: sys_info_init();
    4. 割り込みテーブルをシステム情報にセット: copy_interrupt_table();
    5. カーネルページテーブルを初期化: init_kernel_vm();
    6. mini_uartを初期化: uart_dev_init(19200);
    7. _kmallocを初期化: kmalloc_init();
    8. カーネルイベントを初期化: kev_init();
    9. SDを初期化: sd_init();
    10. _kernel_configを初期化: _load_kernel_config();
    11. mini_uartを再初期化: uart_dev_init(_kernel_config.uart_baud);
    12. カーネルページテーブルをリマップ: reset_kernel_vm();
    13. カーネルページテーブルを再初期化: kmalloc_init();
    14. kmalloc（共有メモリ用）を初期化: kmalloc_vm_init();
    15. その他の割り当て可能なメモリを初期化: init_allocable_mem();
    16. ロゴを表示: logo();
    17. configを表示: show_config();
    18. DMAを初期化: dma_init();
    19. セマフォを初期化: semaphore_init();
    20. 割り込みを初期化: irq_init();
    21. 共有メモリを初期化: shm_init();
    22. プロセスサブシステムを初期化: procs_init();
    23. "/sbin/init"をロード: load_init_proc();
    24. core0のアイドルプロセスを作成: kfork_core_halt(0);
    25. カーネルロックを初期化: kernel_lock_init();
    26. core1-core3について

        1. activedをクリア
        2. アイドルプロセスを作成
        3. coreをstart
        4. activateされるまで待機

    27. タイマーをセット: timer_set_interval(0, _kernel_config.timer_freq);
    28: 割り込みを有効に: __irq_enable();
    29: 停止: halt();

    注: 25-26 はKENEL_SMPが定義されている場合のみ（ここでは該当）

4. "/sbin/init"が選択されて実行

    1. "/sbin/core"を起動: ipcサーバおよびイベントサーバ
    2. "/sbin/vfsd"を起動: vfs処理を行うipcサーバ
    3. "/sbin/sdfsd"を起動: rootfs (ext2) のvfs処理を行うデバイスドライバ
    4. "/etc/init%d.rd" (i=0-7) を実行
    5. "/etc/init.rd"を実行
        ```c
        @/bin/ipcserv /drivers/raspix/uartd         /dev/tty0
        @set_stdio /dev/tty0
        @/bin/ipcserv /drivers/timerd
        @/bin/ipcserv /drivers/ramfsd          /tmp
        @/bin/ipcserv /drivers/nulld           /dev/null
        @/bin/ipcserv /sbin/sessiond
        @/bin/bgrun /bin/session -r -t /dev/tty0
            proc_exec("/bin/login");
                proc_exec("/bin/shell");
        ```