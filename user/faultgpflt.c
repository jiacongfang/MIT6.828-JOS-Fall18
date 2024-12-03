// test user-level fault handler -- just exit when we fault
#include <inc/lib.h>

void handler(struct UTrapframe *utf)
{
    void *addr = (void *)utf->utf_fault_va;
    uint32_t err = utf->utf_err;
    cprintf("i faulted at va %x, err %x\n", addr, err & 7);
    sys_env_destroy(sys_getenvid());
}

void umain(int argc, char **argv)
{
    set_gpflt_handler(handler);
    // make a general protection fault by executing an gerenal protection instruction
    // asm volatile("mov %cr4, %eax; mov %eax, %fs");
    asm volatile("mov $0x23, %ax; ltr %ax");

    cprintf("This line should never be reached!\n");
}