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
    uint64 sem_id;
    uint64 pshared;
    uint64 value;
	
	 // Use semalloc to retrieve an available semaphore index
    sem_id = semalloc();
    if (sem_id< 0)
        return -1;  // Failed to allocate a semaphore
        
    // Use argaddr() to retrieve the semaphore ID
    if (argaddr(0, &sem_id) < 0 || argaddr(1, &pshared) < 0 || argaddr(2, &value) < 0)
        return -1;

    if (sem_id < 0 || sem_id >= NSEM || semtable.sem[sem_id].valid)
        return -1; // Invalid semaphore ID or semaphore already in use

    acquire(&semtable.lock);
    semtable.sem[sem_id].count = value;
    semtable.sem[sem_id].valid = 1;
    release(&semtable.lock);

    // Use copyout to write the sem_id back to user space
    if (copyout(myproc()->pagetable, argaddr(0, &sem_id), (char *)&sem_id, sizeof(sem_id)) < 0){
    	copyout(myproc()->pagetable, argaddr(0, &sem_id), (char *)&sem_id, sizeof(sem_id));
        return -1; // Copyout failed
        }

    return 0;
}



uint64 sys_sem_destroy(void) {
    uint64 sem_addr;

    // Use argaddr() to retrieve the semaphore address
    if (argaddr(0, &sem_addr) < 0)
        return -1;

    int sem_index;

    // Use copyin to safely get the sem_index from user space
    if (copyin(myproc()->pagetable, (char *)&sem_index, sem_addr, sizeof(sem_index)) < 0)
        return -1;

    if (sem_index < 0 || sem_index >= NSEM)
        return -1; // Invalid semaphore index

    acquire(&semtable.lock);

    // Check if the semaphore is valid
    if (!semtable.sem[sem_index].valid) {
        release(&semtable.lock);
        return -1;  // Invalid semaphore or already destroyed
    }

    // Release the semaphore if it's valid
    semtable.sem[sem_index].valid = 0;
    semdealloc(sem_index);
    
    release(&semtable.lock);

    return 0;
}


uint64 sys_sem_wait(void) {
    uint64 sem_addr;

    // Use argaddr() to retrieve the semaphore address
    if (argaddr(0, &sem_addr) < 0)
        return -1;

    int sem_index;

    // Use copyin to safely get the sem_index from user space
    if (copyin(myproc()->pagetable, (char *)&sem_index, sem_addr, sizeof(sem_index)) < 0){
        copyin(myproc()->pagetable, (char *)&sem_index, sem_addr, sizeof(sem_index));
        return -1;
        }

    if (sem_index < 0 || sem_index >= NSEM || !semtable.sem[sem_index].valid)
        return -1; // Invalid semaphore index or semaphore not in use

    acquire(&semtable.sem[sem_index].lock);

    while (semtable.sem[sem_index].count <= 0) {
        sleep(&semtable.sem[sem_index], &semtable.sem[sem_index].lock);
    }

    semtable.sem[sem_index].count--;
    release(&semtable.sem[sem_index].lock);

    return 0;
}

uint64 sys_sem_post(void) {
    uint64 sem_id;

    if (argaddr(0, &sem_id) < 0)
        return -1;

    if (sem_id < 0 || sem_id >= NSEM || !semtable.sem[sem_id].valid)
        return -1; // Invalid semaphore ID or semaphore not in use

    acquire(&semtable.sem[sem_id].lock);

    // Increment the value of the semaphore by one
    semtable.sem[sem_id].count++;

    // Use copyin to access the user's sem_t value
    uint64 sem_t_value;
    if (copyin(myproc()->pagetable, (char *)&sem_t_value, argaddr(0, &sem_id), sizeof(sem_t_value)) < 0) {
        copyin(myproc()->pagetable, (char *)&sem_t_value, argaddr(0, &sem_id), sizeof(sem_t_value));
        release(&semtable.sem[sem_id].lock);
        return -1; // Copyin failed
    }

    // Wake up waiting processes
    if (sem_t_value > 0) {
        wakeup(&semtable.sem[sem_id]);
    }

    release(&semtable.sem[sem_id].lock);

    return 0;
}




