#include <kernel/proc.h>
#include <kernel/kernel.h>
#include <kprintf.h>
#include <kernel/system.h>
#include <kernel/core.h>
#include <stddef.h>

int32_t schedule(context_t* ctx) {
    // ZOMBIEプロセスを解放
	proc_zombie_funeral();

	uint32_t core = get_core_id();
	proc_t* halt_proc = NULL;
	if(_cpu_cores[core].halt_proc != NULL) {
		/*　実行可能なプロセスがない場合はWAIT状態で
            中断するhaltプロセスを使用するのでその準備
		*/
		halt_proc = _cpu_cores[core].halt_proc;
		halt_proc->info.state = WAIT;
		halt_proc->info.wait_for = 0;
	}

    // 次に実行すべきプロセスを取得
	proc_t* next = proc_get_next_ready();
    // 次に実行すべきプロセスがない場合はhaltプロセスを選択
	if(next == NULL && halt_proc != NULL) {
		next = halt_proc;
	}

    // 次のプロセスにスイッチ
	if(next != NULL) {
		next->info.state = RUNNING;
		proc_switch(ctx, next, false);
		return 0;
	}

    // panic: 実行すべきプロセスがない
	printf("Panic: none proc to be scheduled!\n");
	halt();
	return 0;
}
