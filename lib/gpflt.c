// User-level zero-divide exception support

#include <inc/lib.h>

void (*_gpflt_handler)(struct UTrapframe *utf);

// Assembly language zerodiv entrypoint defined in lib/zerodiventry.S.
extern void _gpflt_upcall(void);

//
// Set the general protection fault handler function.
// If there isn't one yet, _gpflt_handler will be 0.
// The first time we register a handler, we need to
// allocate an exception stack (one page of memory with its top
// at UXSTACKTOP), and tell the kernel to call the assembly-language
// _gpflt_upcall routine when a general protection fault occurs.
//
void set_gpflt_handler(void (*handler)(struct UTrapframe *utf))
{
    int r;

    if (_gpflt_handler == 0)
    {
        // First time through!
        // LAB 4: Your code here.

        // Allocate an exception stack.
        if ((r = sys_page_alloc(0, (void *)(UXSTACKTOP - PGSIZE), PTE_U | PTE_W | PTE_P)) < 0)
        {
            panic("set_gpflt_handler: sys_page_alloc: %e", r);
        }
        // Register the assembly-language handler.
        if ((r = sys_env_set_gpflt_upcall(0, _gpflt_upcall)) < 0)
        {
            panic("set_gpflt_handler: sys_env_set_gpflt_upcall: %e", r);
        }
    }

    // Save handler pointer for assembly to call.
    _gpflt_handler = handler;
}