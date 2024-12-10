
#include <inc/lib.h>

void exit(void)
{
	close_all();
	// cprintf("exit\n");
	sys_env_destroy(0);
}
