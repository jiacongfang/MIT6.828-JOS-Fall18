// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/pmap.h>
#include <inc/mmu.h>
#include <kern/trap.h>

#define CMDBUF_SIZE 80 // enough for one VGA text line

struct Command
{
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char **argv, struct Trapframe *tf);
};

static struct Command commands[] = {
	{"help", "Display this list of commands", mon_help},
	{"kerninfo", "Display information about the kernel", mon_kerninfo},
	{"backtrace", "Back trace the function call frames", mon_backtrace},
	{"showmappings",
	 "Display the physical page mappings that apply to [start_va, end_va]",
	 mon_showmappings},
	{"setpermission",
	 "Set, clear or change the permissions of mappping at [va]",
	 mon_setpermission},
	{"dumpmem",
	 "Dump the contents of a range of memory given either a virtual or physical address range.",
	 mon_dumpmem},
	{"step", "Single step the kernel", mon_step},
	{"continue", "Continue the kernel", mon_continue},
};

/***** Implementations of basic kernel monitor commands *****/

int mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
			ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
	/* Here are some test codes for prev exercise.
	// 8.3
	// int x = 1, y = 3, z = 4;
	// cprintf("x %d, y %x, z %d\n", x, y, z);
	// return 0;

	// 8.4
	// unsigned int i = 0x00646c72;
	// cprintf("H%x Wo%s", 57616, &i);

	// 8.5
	// cprintf("x=%d y=%d", 3);

	// test the color mode
	// cprintf("This is a test for challenge\n");
	// cprintf("%s", "\033[31;42mHello Red World!\033[0m\n");
	// cprintf("%s", "\033[32;43mHello Green World!\033[0m\n");
	// cprintf("%s", "\033[33;44mHello Yellow World!\033[0m\n");
	// cprintf("%s", "\033[34;45mHello Blue World!\033[0m\n");
	// cprintf("%s", "\033[35;46mHello Magenta World!\033[0m\n");
	// cprintf("%s", "\033[36;47mHello Cyan World!\033[0m\n");
	// cprintf("%s", "\033[a;31m Test Invalid Code!\033[0m\n");
	*/

	// Exercise 11
	uint32_t ebp = read_ebp();
	while (ebp)
	{
		uint32_t eip = *((uint32_t *)(ebp + 4));
		// cprintf("ebp %08x  eip %08x  args %08x %08x %08x %08x %08x\n",
		// 		ebp,
		// 		eip,
		// 		*((uint32_t *)(ebp + 8)),
		// 		*((uint32_t *)(ebp + 12)),
		// 		*((uint32_t *)(ebp + 16)),
		// 		*((uint32_t *)(ebp + 20)),
		// 		*((uint32_t *)(ebp + 24)));

		if ((uint32_t)ebp == 0xeebfdff0)
		{ // handle the stack created by _start
			cprintf("  ebp %08x  eip %08x  args %08x %08x\n",
					ebp,
					eip,
					*((uint32_t *)(ebp + 8)),
					*((uint32_t *)(ebp + 12)));
		}
		else
		{
			cprintf("  ebp %08x  eip %08x  args %08x %08x %08x %08x %08x\n",
					ebp,
					eip,
					*((uint32_t *)(ebp + 8)),
					*((uint32_t *)(ebp + 12)),
					*((uint32_t *)(ebp + 16)),
					*((uint32_t *)(ebp + 20)),
					*((uint32_t *)(ebp + 24)));
		}

		struct Eipdebuginfo info;
		debuginfo_eip(eip, &info);
		// Example kern/monitor.c:143: monitor+106
		cprintf("       %s:%d: %.*s+%d\n",
				info.eip_file,
				info.eip_line,
				info.eip_fn_namelen,
				info.eip_fn_name,
				eip - info.eip_fn_addr);
		ebp = *((uint32_t *)(ebp));
	}
	return 0;
}

/***** Lab 2: Kernel monitor about memory management *****/

/***** Lab 3: Challenge 2 Add Support to Step and Continue *****/
int mon_step(int argc, char **argv, struct Trapframe *tf)
{
	// Check if the monitor is called by the trap handler in user mode.
	if (!(tf && ((tf->tf_cs & 0x3) == 0x3) && (tf->tf_trapno == T_BRKPT || tf->tf_trapno == T_DEBUG)))
	{
		cprintf("The monitor is not called by the trap handler in user mode.\n");
		return 0;
	}
	cprintf("Step the kernel\n");
	tf->tf_eflags |= FL_TF;
	return -1;
}

int mon_continue(int argc, char **argv, struct Trapframe *tf)
{
	// Check if the monitor is called by the trap handler in user mode.
	if (!(tf && ((tf->tf_cs & 0x3) == 0x3) && (tf->tf_trapno == T_BRKPT || tf->tf_trapno == T_DEBUG)))
	{
		cprintf("The monitor is not called by the trap handler in user mode.\n");
		return 0;
	}
	tf->tf_eflags &= ~FL_TF;
	return -1;
}

// Usage: showmappings start_va end_va
int mon_showmappings(int argc, char **argv, struct Trapframe *tf)
{
	if (argc != 3)
	{
		cprintf("Usage: showmappings <start_va> <end_va>\n");
		return 0;
	}
	uintptr_t va_start = (uintptr_t)strtol(argv[1], 0, 0);
	uintptr_t va_end = (uintptr_t)strtol(argv[2], 0, 0);

	// Align to  PGSIZE
	va_start = ROUNDDOWN(va_start, PGSIZE);
	va_end = ROUNDUP(va_end, PGSIZE);

	physaddr_t phys;

	for (; va_start <= va_end; va_start += PGSIZE)
	{
		pte_t *pte = pgdir_walk(kern_pgdir, (void *)va_start, 0);
		if (!(pte && (*pte & PTE_P)))
		{
			cprintf("VisAddr: 0x%08x, PhysAddr: no mappings\n", va_start);
		}
		else
		{
			phys = PTE_ADDR(*pte);
			cprintf("VisAddr: 0x%08x, PhysAddr: 0x%08x, PTE_U: %d, PTE_W: %d\n",
					va_start, phys, (*pte & PTE_U) > 0, (*pte & PTE_W) > 0);
		}
	}
	return 0;
}

// Usage: setpermission [va] [U=0/U=1] [W=0/W=1]
int mon_setpermission(int argc, char **argv, struct Trapframe *tf)
{
	static const char *help_msg_1 = "Usage: setpermission [va] [U=0/U=1] [W=0/W=1]\n";
	if (argc != 4)
	{
		cprintf(help_msg_1);
		return 0;
	}

	uintptr_t va = ROUNDDOWN((uintptr_t)strtol(argv[1], 0, 0), PGSIZE);

	uint16_t perm;

	if (0 == strcmp(argv[2], "U=1"))
		perm = PTE_U;
	else if (0 == strcmp(argv[2], "U=0"))
		perm = 0x000;
	else
	{
		cprintf(help_msg_1);
		return 0;
	}

	if (0 == strcmp(argv[3], "W=1"))
		perm |= PTE_W;
	else if (0 == strcmp(argv[3], "W=0"))
	{
	}
	else
	{
		cprintf(help_msg_1);
		return 0;
	}

	pte_t *pte = pgdir_walk(kern_pgdir, (void *)va, 0);
	if (!(pte && (*pte & PTE_P)))
		cprintf("VisAddr: 0x%08x, PhysAddr: no mappings\n", va);
	else
	{
		*pte = (*pte & ~0xFFF) | perm | PTE_P;
	}
	return 0;
}

// Usage: dumpmem [-p/-v] start_va length
int mon_dumpmem(int argc, char **argv, struct Trapframe *tf)
{
	static const char *help_msg_2 =
		"Usage: dumpmem [-p/-v] start_addr length\n"
		"\t -p : start_addr is physical address\n"
		"\t -v : start_addr is visual address.\n"
		"\t length: must be greater than 0\n";

	if (argc != 4)
	{
		cprintf(help_msg_2);
		return 0;
	}

	bool visual_flag;

	if (0 == strcmp(argv[1], "-p"))
		visual_flag = false;
	else if (0 == strcmp(argv[1], "-v"))
		visual_flag = true;
	else
	{
		cprintf(help_msg_2);
		return 0;
	}

	uint32_t start_addr = (uint32_t)strtol(argv[2], 0, 0);
	size_t length = (size_t)strtol(argv[3], 0, 0);
	uint32_t end_addr = start_addr + length;
	uint32_t next; // Used to check if the range cross page.
	pte_t *pte;

	if (length == 0)
	{
		cprintf("Length must be greater than zero.\n");
		return 0;
	}

	if (visual_flag)
	{ // Visual address
		while (start_addr < end_addr)
		{
			pte = pgdir_walk(kern_pgdir, (void *)start_addr, 0);

			if (!pte) // The relevant page table page might not exist yet.
			{
				next = (uint32_t)PGADDR(PDX(start_addr) + 1, 0, 0); // The next page dir entry
				if (end_addr < next)
					for (; start_addr < end_addr; start_addr++)
						cprintf("VisAddr: 0x%08x, PhysAddr: no mappings, None\n", start_addr);
				else
					for (; start_addr < next; start_addr++)
						cprintf("VisAddr: 0x%08x, PhysAddr: no mappings, None\n", start_addr);
			}
			else if (!(*pte & PTE_P)) // No such PTE
			{
				next = (uint32_t)PGADDR(PDX(start_addr), PTX(start_addr) + 1, 0); // The next page table entry
				if (end_addr < next)
					for (; start_addr < end_addr; start_addr++)
						cprintf("VisAddr: 0x%08x, PhysAddr: no mappings, None\n", start_addr);
				else
					for (; start_addr < next; start_addr++)
						cprintf("VisAddr: 0x%08x, PhysAddr: no mappings, None\n", start_addr);
			}
			else // PTE exists
			{
				next = (uint32_t)PGADDR(PDX(start_addr), PTX(start_addr) + 1, 0); // The next page table entry
				if (end_addr < next)
					for (; start_addr < end_addr; start_addr++)
						cprintf("VisAddr: 0x%08x, PhysAddr:0x%08x, %02x\n",
								start_addr, PTE_ADDR(*pte), *(uint8_t *)start_addr);
				else
					for (; start_addr < next; start_addr++)
						cprintf("VisAddr: 0x%08x, PhysAddr:0x%08x, %02x\n",
								start_addr, PTE_ADDR(*pte), *(uint8_t *)start_addr);
			}
		}
	}
	else
	{
		if (end_addr > npages * PGSIZE || end_addr > 0xffffffff - KERNBASE + 1)
		{
			cprintf("Out of memory, please change address and length.\n");
			return 0;
		}

		// Use KADDR to get the response kernel visual address
		for (; start_addr <= end_addr; start_addr++)
		{
			cprintf("PA: 0x%08x, %02x\n", start_addr, *(uint8_t *)KADDR(start_addr));
		}
		return 0;
	}
	return 0;
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1)
	{
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS - 1)
		{
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++)
	{
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);
	while (1)
	{
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
