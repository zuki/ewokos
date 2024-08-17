#ifndef SYS_SIGNAL_H
#define SYS_SIGNAL_H

#include <signals.h>

typedef void(*signal_handler_t)(int signo, void* p);

void             sys_sig_default(int sig_no, void* p);
void             sys_sig_ignore(int sig_no, void* p);
void             sys_signal_init(void);
signal_handler_t sys_signal(int sig_no, signal_handler_t handler, void* p);

#undef SIG_IGN
#define SIG_IGN sys_sig_ignore
#undef SIG_DFL
#define SIG_DFL sys_sig_default

#endif
