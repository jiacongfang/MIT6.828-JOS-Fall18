// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW 0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *)utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;
	int envid = sys_getenvid();

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if (!(err & FEC_WR) || !(uvpt[PGNUM(addr)] & PTE_COW))
		panic("pgfault: not a write to a copy-on-write page\n");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.

	// Step 1: Allocate a new page and map it at PFTEMP
	if ((r = sys_page_alloc(envid, PFTEMP, PTE_P | PTE_U | PTE_W)) < 0)
		panic("sys_page_alloc: %e", r);

	// Step 2: copy the data from the old page to the new page
	memmove(PFTEMP, ROUNDDOWN(addr, PGSIZE), PGSIZE);

	// Step 3: move the new page to the old page's address
	if ((r = sys_page_map(envid, PFTEMP, envid, ROUNDDOWN(addr, PGSIZE), PTE_P | PTE_U | PTE_W)) < 0)
		panic("sys_page_map: %e", r);

	// Done. Unmap the temporary page.
	if ((r = sys_page_unmap(envid, PFTEMP)) < 0)
		panic("sys_page_unmap: %e", r);

	// print the permission of the page
	// cprintf("pgfault: envid = %08x, fault_va = %08x, utf=%08x\n", envid, addr, utf);
	// cprintf("pgfault: cur_env=%08x, PTE_W = %d, PTE_COW = %d\n", thisenv->env_id, uvpt[PGNUM(addr)] & PTE_W, uvpt[PGNUM(addr)] & PTE_COW);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
	int parent_envid = sys_getenvid();
	// LAB 4: Your code here.
	assert(uvpt[pn] & PTE_P);
	assert(uvpt[pn] & PTE_U);

	// check if the page is writable or copy-on-write
	if ((uvpt[pn] & PTE_W) || (uvpt[pn] & PTE_COW))
	{
		// map the page copy-on-write into the address space of the child
		if ((r = sys_page_map(parent_envid, (void *)(pn * PGSIZE), envid, (void *)(pn * PGSIZE), PTE_P | PTE_U | PTE_COW)) < 0)
			return r;

		// remap the page copy-on-write in its own address space
		if ((r = sys_page_map(parent_envid, (void *)(pn * PGSIZE), parent_envid, (void *)(pn * PGSIZE), PTE_P | PTE_U | PTE_COW)) < 0)
		{
			cprintf("r = %d\n", r);
			return r;
		}
	}
	else
	{
		// map the page as read-only
		if ((r = sys_page_map(parent_envid, (void *)(pn * PGSIZE), envid, (void *)(pn * PGSIZE), PTE_P | PTE_U)) < 0)
			return r;
	}
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	envid_t envid;
	int r;

	// 1. install `pgfault` as the page fault handler
	set_pgfault_handler(pgfault);

	// 2. Create a child.
	envid = sys_exofork();
	if (envid < 0)
		panic("sys_exofork: %e", envid);
	if (envid == 0)
	{
		// We're the child.
		thisenv = &envs[ENVX(sys_getenvid())];
		// cprintf("fork: child envid = %08x\n", thisenv->env_id);
		// return 0 to the child
		return 0;
	}

	// We're the parent.
	// 3. For each writable or copy-on-write page in its address space below UTOP,
	// the parent calls duppage, which should map the page copy-on-write into
	// the address space of the child and then remap the page copy-on-write in its own address space.
	for (uint32_t add = 0; add < UTOP - PGSIZE; add += PGSIZE)
	{
		// uvpd: 	pointer to the current page directory
		// uvpd[i]: the page directory entry for the page table that maps virtual address i
		// uvpt: 	pointer to the page table
		// uvpt[i]: the page table entry for page i.

		// check if the corresponding PDE and PTE are present
		if ((uvpd[PDX(add)] & PTE_P) == PTE_P && (uvpt[PGNUM(add)] & PTE_P) == PTE_P)
		{
			if ((r = duppage(envid, PGNUM(add))) < 0)
				return r;
		}
	}
	extern void _pgfault_upcall(void);

	// allocate a fresh page in the child for the exception stack.
	if ((r = sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), PTE_P | PTE_U | PTE_W)) < 0)
		panic("sys_page_alloc: %e", r);

	// 4. set the user page fault entrypoint for the child
	if ((r = sys_env_set_pgfault_upcall(envid, _pgfault_upcall)) < 0)
		panic("sys_env_set_pgfault_upcall: %e", r);

	// 5. mark the child as runnable
	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", r);

	// 6. return the child's envid to the parent
	return envid;
}

// map the page of the parent to the child, without modifying the permissions
static int
sduppage(envid_t envid, unsigned pn)
{
	int parent_id = sys_getenvid();
	void *add = (void *)(pn * PGSIZE);
	int r;
	if ((r = sys_page_map(parent_id, add, envid, add, uvpt[pn] & 0xfff & PTE_SYSCALL)) < 0)
		panic("sduppage (sys_page_map): %e", r);

	return 0;
}

// Challenge!
int sfork(void)
{
	envid_t envid;
	int r;

	// 1. install `pgfault` as the page fault handler
	set_pgfault_handler(pgfault);

	// 2. Create a child.
	envid = sys_exofork();
	if (envid < 0)
		panic("sys_exofork: %e", envid);
	if (envid == 0)
	{
		// We're the child.
		thisenv = &envs[ENVX(sys_getenvid())];
		// cprintf("fork: child envid = %08x\n", thisenv->env_id);
		// return 0 to the child
		return 0;
	}

	// We're the parent.
	// 从栈底开始向下复制，在用户栈的部分使用 duppage(COW), 在其他部分使用 sduppage(共享内存)
	uint32_t add;
	for (add = USTACKTOP - PGSIZE; add >= UTEXT; add -= PGSIZE)
	{
		if ((uvpd[PDX(add)] & PTE_P) == PTE_P && (uvpt[PGNUM(add)] & PTE_P) == PTE_P)
		{
			if ((r = duppage(envid, PGNUM(add))) < 0)
				return r;
		}
		else
			break; // 抵达栈顶, 剩下的部分直接共享内存
	}

	for (; add >= UTEXT; add -= PGSIZE)
	{
		if ((uvpd[PDX(add)] & PTE_P) == PTE_P && (uvpt[PGNUM(add)] & PTE_P) == PTE_P)
		{
			if ((r = sduppage(envid, PGNUM(add))) < 0)
				return r;
		}
	}

	extern void _pgfault_upcall(void);

	// allocate a fresh page in the child for the exception stack.
	if ((r = sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), PTE_P | PTE_U | PTE_W)) < 0)
		panic("sys_page_alloc: %e", r);

	// 4. set the user page fault entrypoint for the child
	if ((r = sys_env_set_pgfault_upcall(envid, _pgfault_upcall)) < 0)
		panic("sys_env_set_pgfault_upcall: %e", r);

	// 5. mark the child as runnable
	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", r);

	// 6. return the child's envid to the parent
	return envid;
}
