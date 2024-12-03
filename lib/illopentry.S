#include <inc/mmu.h>
#include <inc/memlayout.h>

.text
.globl _illop_upcall
_illop_upcall:
    pushl %esp
    movl _illop_handler, %eax
    call *%eax
    addl $4, %esp

    ret