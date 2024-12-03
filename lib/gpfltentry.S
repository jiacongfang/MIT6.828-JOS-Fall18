#include <inc/mmu.h>
#include <inc/memlayout.h>

.text
.globl _gpflt_upcall
_gpflt_upcall:
    pushl %esp
    movl _gpflt_upcall, %eax
    call *%eax
    addl $4, %esp

    ret