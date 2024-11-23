#include <inc/mmu.h>
#include <inc/memlayout.h>

.text
.globl _zerodiv_upcall
_zerodiv_upcall:
    pushl %esp
    movl _zerodiv_handler, %eax
    call *%eax
    addl $4, %esp

    ret