#include <ewoksys/ipc.h>
#include <ewoksys/syscall.h>
#include <ewoksys/core.h>
#include <ewoksys/proc.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/shm.h>

#ifdef __cplusplus
extern "C"
{
#endif

	static int ipc_setup(ipc_handle_t handle, void *p, int flags)
	{
		return syscall3(SYS_IPC_SETUP, (ewokos_addr_t)handle, (ewokos_addr_t)p, (ewokos_addr_t)flags);
	}

	static int ipc_set_return(uint32_t ipc_id, const proto_t *pkg)
	{
		syscall2(SYS_IPC_SET_RETURN, (ewokos_addr_t)ipc_id, (ewokos_addr_t)pkg);
		return 0;
	}

	static void ipc_end(void)
	{
		syscall0(SYS_IPC_END);
	}

	static inline int ipc_ping(int pid)
	{
		return syscall1(SYS_IPC_PING, (int32_t)pid);
	}

	inline void ipc_ready(void)
	{
		syscall0(SYS_IPC_READY);
	}

	void ipc_wait_ready(int pid)
	{
		while (1)
		{
			if (ipc_ping(pid) == 0)
				break;
			proc_usleep(100000);
		}
	}

	int ipc_disable(void)
	{
		while (true)
		{
			int res = syscall0(SYS_IPC_DISABLE);
			if (res == 0)
				break;
			sleep(0);
		}
		return 0;
	}

	void ipc_enable(void)
	{
		syscall0(SYS_IPC_ENABLE);
	}

	static int32_t ipc_get_info(uint32_t ipc_id, int32_t *pid, int32_t *call_id, proto_t *arg)
	{
		int32_t ipc_info[2];
		int32_t res = syscall3(SYS_IPC_GET_ARG, (ewokos_addr_t)ipc_id, (ewokos_addr_t)ipc_info, (ewokos_addr_t)arg);
		*pid = ipc_info[0];
		*call_id = ipc_info[1];
		return res;
	}

	inline int ipc_call(int to_pid, int call_id, const proto_t *ipkg, proto_t *opkg)
	{
		if (to_pid < 0)
			return -1;

		int ipc_id = 0;
		while (true) {
			if (opkg == NULL)
				call_id |= IPC_NON_RETURN;
			ipc_id = syscall3(SYS_IPC_CALL, (ewokos_addr_t)to_pid, (ewokos_addr_t)call_id, (ewokos_addr_t)ipkg);

			if (ipc_id == -1)
				continue;
			if (ipc_id == 0) {
				return -1;
			}
			break;
		}

		if (opkg == NULL)
			return 0;

		int res = -1;
		PF->clear(opkg);
		while (true) {
			res = syscall3(SYS_IPC_GET_RETURN, (ewokos_addr_t)to_pid, (ewokos_addr_t)ipc_id, (ewokos_addr_t)opkg);
			if(res == 0)
				break;

			if (res == -1) // retry
				continue;

			if (res < 0) // error!
				return -1;

			if (res > 0) //opkg not big enough, must resize it.
				PF->reserve(opkg, res);

			res = syscall3(SYS_IPC_GET_RETURN, (ewokos_addr_t)to_pid, (ewokos_addr_t)ipc_id, (ewokos_addr_t)opkg);
			break;
		}
		return res;
	}

	inline int ipc_call_wait(int to_pid, int call_id, const proto_t *ipkg)
	{
		proto_t out;
		PF->init(&out);
		int res = ipc_call(to_pid, call_id, ipkg, &out);
		PF->clear(&out);
		return res;
	}

	/*----ipc server ------*/

	int ipc_serv_reg(const char *ipc_serv_id)
	{
		int core_pid = syscall0(SYS_CORE_PID);
		if (core_pid < 0)
			return -1;

		int res = -1;
		proto_t in, out;
		PF->init(&out);

		PF->init(&in)->adds(&in, ipc_serv_id);
		if (ipc_call(core_pid, CORE_CMD_IPC_SERV_REG, &in, &out) == 0)
		{
			res = proto_read_int(&out);
		}

		PF->clear(&in);
		PF->clear(&out);
		return res;
	}

	int ipc_serv_get(const char *ipc_serv_id)
	{
		int core_pid = syscall0(SYS_CORE_PID);
		if (core_pid < 0)
			return -1;

		int res = -1;
		proto_t in, out;
		PF->init(&out);

		PF->init(&in)->adds(&in, ipc_serv_id);
		if (ipc_call(core_pid, CORE_CMD_IPC_SERV_GET, &in, &out) == 0)
		{
			res = proto_read_int(&out);
		}

		PF->clear(&in);
		PF->clear(&out);
		return res;
	}

	int ipc_serv_unreg(const char *ipc_serv_id)
	{
		int core_pid = syscall0(SYS_CORE_PID);
		if (core_pid < 0)
			return -1;

		int res = -1;
		proto_t in, out;
		PF->init(&out);

		PF->init(&in)->adds(&in, ipc_serv_id);
		if (ipc_call(core_pid, CORE_CMD_IPC_SERV_UNREG, &in, &out) == 0)
		{
			res = proto_read_int(&out);
		}

		PF->clear(&in);
		PF->clear(&out);
		return res;
	}

	static ipc_serv_handle_t _ipc_serv_handle;
	static ipc_handled_t _ipc_handled;

	static void handle_ipc(uint32_t ipc_id, void *p)
	{
		int32_t pid, cmd;
		proto_t in;
		PF->init(&in);

		int res = ipc_get_info(ipc_id, &pid, &cmd, &in);
		if(res < 0) {
			ipc_end();
			return;
		}

		if(res > 0) { //in not big enough, have to resize it
			PF->reserve(&in, res);
			if(ipc_get_info(ipc_id, &pid, &cmd, &in) != 0) {
				ipc_end();
				return;
			}
		}

		proto_t out;
		PF->init(&out);
		_ipc_serv_handle(pid, (cmd & IPC_NON_RETURN_MASK), &in, &out, p);

		PF->clear(&in);

		if ((cmd & IPC_NON_RETURN) == 0)
		{ // need return
			ipc_set_return(ipc_id, &out);
		}
		PF->clear(&out);

		if (_ipc_handled != NULL)
			_ipc_handled(p);
		ipc_end();
	}

	int ipc_serv_run(ipc_serv_handle_t handle, ipc_handled_t handled, void *p, int flags)
	{
		_ipc_serv_handle = handle;
		_ipc_handled = handled;

		int ret = ipc_setup(handle_ipc, p, flags);
		if (ret == 0)
			ipc_ready();

		if ((flags & IPC_NON_BLOCK) == 0)
			proc_block_by(getpid(), 0);
		return ret;
	}

#ifdef __cplusplus
}
#endif
