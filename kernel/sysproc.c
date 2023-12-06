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
  int newsz;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  newsz=addr+n;
  if(!(newsz < TRAPFRAME)){
  return -1;
  }
   myproc()->sz=newsz;
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
  int pages=countPage();
  
  return pages*4096;
}
//Modified for HW6
uint64 sys_sem_init(void) {
    int key;

    if (argint(0, &key) < 0)
        return -1;

    // Allocate a new semaphore
    int sem_index = semalloc();
    if (sem_index < 0)
        return -1;  // Failed to allocate a semaphore

    // Initialize the semaphore with the provided key
    acquire(&semtable.lock);
    semtable.sem[sem_index].count = key;
    semtable.sem[sem_index].valid = 1;
    release(&semtable.lock);

    return sem_index;
}

uint64 sys_sem_destroy(void) {
    int sem_index;

    if (argint(0, &sem_index) < 0)
        return -1;

    if (sem_index < 0 || sem_index >= NSEM)
        return -1;  // Invalid semaphore index

    acquire(&semtable.lock);

    // Release the semaphore if it's valid
    if (semtable.sem[sem_index].valid) {
        semtable.sem[sem_index].valid = 0;
        release(&semtable.lock);
        semdealloc(sem_index);
        return 0;
    }

    release(&semtable.lock);
    return -1;  // Invalid semaphore or already destroyed
}

uint64 sys_sem_wait(void) {
    int sem_index;
	int total=0;
    if (argint(0, &sem_index) < 0)
        return -1;

    if (sem_index < 0 || sem_index >= NSEM)
        return -1;  // Invalid semaphore index

    acquire(&semtable.lock);

    // Check if the semaphore is valid
    if (!semtable.sem[sem_index].valid) {
        release(&semtable.lock);
        return -1;  // Invalid semaphore
    }

    // Decrement the value of the semaphore by one
    semtable.sem[sem_index].count--;

    while (semtable.sem[sem_index].count < 0) {
        // Wait if the value of the semaphore is negative
        sleep(&semtable.sem[sem_index], &semtable.lock);
    }

    total++;  // Increment the global total
    release(&semtable.lock);

    return total;
}

uint64 sys_sem_post(void) {
    int sem_index;

    if (argint(0, &sem_index) < 0)
        return -1;

    if (sem_index < 0 || sem_index >= NSEM)
        return -1;  // Invalid semaphore index

    acquire(&semtable.lock);

    // Check if the semaphore is valid
    if (!semtable.sem[sem_index].valid) {
        release(&semtable.lock);
        return -1;  // Invalid semaphore
    }

    // Increment the value of the semaphore by one
    semtable.sem[sem_index].count++;

    // Wake up one waiting process if there are any
    if (semtable.sem[sem_index].count <= 0) {
        wakeup(&semtable.sem[sem_index]);
    }

    release(&semtable.lock);

    return 0;
}


