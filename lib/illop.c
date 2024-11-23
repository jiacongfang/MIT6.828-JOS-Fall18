// User-level zero-divide exception support

#include <inc/lib.h>

void (*_illop_handler)(struct UTrapframe *utf);

// Assembly language zerodiv entrypoint defined in lib/zerodiventry.S.
extern void _illop_upcall(void);

//
// Set the illegal opcode handler function.
// If there isn't one yet, _illop_handler will be 0.
// The first time we register a handler, we need to
// allocate an exception stack (one page of memory with its top
// at UXSTACKTOP), and tell the kernel to call the assembly-language
// _illop_upcall routine when a zero-divide exception occurs.
//
void set_illop_handler(void (*handler)(struct UTrapframe *utf))
{
    int r;

    if (_illop_handler == 0)
    {
        // First time through!
        // LAB 4: Your code here.

        // Allocate an exception stack.
        if ((r = sys_page_alloc(0, (void *)(UXSTACKTOP - PGSIZE), PTE_U | PTE_W | PTE_P)) < 0)
        {
            panic("set_illop_handler: sys_page_alloc: %e", r);
        }
        // Register the assembly-language handler.
        if ((r = sys_env_set_illop_upcall(0, _illop_upcall)) < 0)
        {
            panic("set_illop_handler: sys_env_set_illop_upcall: %e", r);
        }
    }

    // Save handler pointer for assembly to call.
    _illop_handler = handler;
}