#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "pstat.h"
#include "limits.h"
#define MAXEFFPRIORITY 99

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}


uint64
sys_wait2(void)
{
  uint64 p, p2;
  if(argaddr(0, &p) < 0 || argaddr(1, &p2) < 0 )
    return -1;
  return wait2(p, p2);
}
// return the number of active processes in the system
// fill in user-provided data structure with pid,state,sz,ppid,name


uint64
sys_getpriority(void)
{
  return myproc()->priority;
}

uint64
sys_getprocs(void)
{
    uint64 addr;  // user pointer to struct pstat

    if (argaddr(0, &addr) < 0)
        return -1;

    return procinfo(addr);
}

uint64
sys_setpriority(void)
{
  int newPriority;
  if (argint(0, &newPriority) < 0)
    return -1;

  
	
  // Lock the process to ensure safe modification of the priority
   // acquire(&p->lock);

    // Validate newPriority
    if (newPriority < 0 || newPriority > MAXEFFPRIORITY) {
       // release(&p->lock); // Release the lock before returning
        return -1; // Invalid priority value
    }

    // Set the process's new priority
    myproc()->priority=newPriority;

    // Release the lock and return 0 as a success indicator
    //release(&p->lock);
  return 0; //succes
}
//Modified for HW4

uint64 sys_freepmem(void)
{
  int n;
    struct proc *curproc = myproc();  // Get the current process

   
    // Read the number of pages to free from the user
    if (argint(0, &n) < 0)
        return -1;  // Invalid argument

    // Perform the memory freeing logic
    for (int i = 0; i < n; i++) {
        char *mem = kalloc();  // Allocate a page of physical memory
        if (mem == 0)
            return -1;  // Out of memory

        // Fill with junk and free the page (as in kfree)
        memset(mem, 1, PGSIZE);

        
        ((struct run*)mem)->next = kmem.freelist;
        kmem.freelist = (struct run*)mem;
    }

    return 0;  // Success
}


