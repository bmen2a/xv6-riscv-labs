#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "pstat.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[], userret[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

void
trapinit(void)
{
  initlock(&tickslock, "time");
}

// set up to take exceptions and traps while in the kernel.
void
trapinithart(void)
{
  w_stvec((uint64)kernelvec);
}

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void
usertrap(void)
{
  int which_dev = 0;
  int newsz = myproc()->sz;

  if ((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // send interrupts and exceptions to kerneltrap(),
  // since we're now in the kernel.
  w_stvec((uint64)kernelvec);

  struct proc *p = myproc();
  p->cputime++;   // Modified for HW2
  // save user program counter.
  p->trapframe->epc = r_sepc();

  if (r_scause() == 8) {
    // system call

    if (p->killed)
      exit(-1);

    // sepc points to the ecall instruction,
    // but we want to return to the next instruction.
    p->trapframe->epc += 4;

    // an interrupt will change sstatus &c registers,
    // so don't enable until done with those registers.
    intr_on();

    syscall();

    // after allocating a physical memory frame, clear contents of the page
    // mappages(p->pagetable, virtual address or stval, page size,
    // newly allocated physical frame addr obtained from kalloc(),
    // PERMISSIONS R/W/X/U)
  }if (r_scause() == 13 || r_scause() == 15) {
    if (r_stval() < newsz) {
      // Handle load or store fault
      // Calculate the virtual page address containing the faulting address
      uint64 faulting_addr = r_stval();
      uint64 virtual_page = PGROUNDDOWN(faulting_addr);

      // Acquire the process lock before accessing proc struct fields
      //acquire(&p->lock);
      char *mem = kalloc();  // Allocate a physical memory frame
      //memset(mem, 0, PGSIZE);
      if (mem == 0) {
        // Handle allocation failure, e.g., by returning an error or killing the process
        printf("Out of physical memory\n");
        //printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
   // printf("Panic:");
        p->killed = 1;

        // Release the process lock before exiting
        //release(&p->lock);
        return;
      }
	virtual_page = PGROUNDDOWN(faulting_addr);

      // Install the page table mapping
      if (mappages(p->pagetable, virtual_page, PGSIZE, (uint64)mem, PTE_R | PTE_W | PTE_X | PTE_U) < 0) {
        // Free the physical memory frame in case of an error
        kfree(mem);
        printf("Failed to map pages\n");
        p->killed = 1;
        return;
      }

      // Release the process lock
      //release(&p->lock);
    }
  } else if ((which_dev = devintr()) != 0) {
    // ok
    // Release the process lock
    //release(&p->lock);
  } else {
    printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
    printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
    p->killed = 1;
  }

  if (p->killed)
    exit(-1);

  // give up the CPU if this is a timer interrupt.
  if (which_dev == 2)
    yield();

  usertrapret();
}

//
// return to user space
//
void
usertrapret(void)
{
  struct proc *p = myproc();

  // we're about to switch the destination of traps from
  // kerneltrap() to usertrap(), so turn off interrupts until
  // we're back in user space, where usertrap() is correct.
  intr_off();

  // send syscalls, interrupts, and exceptions to trampoline.S
  w_stvec(TRAMPOLINE + (uservec - trampoline));

  // set up trapframe values that uservec will need when
  // the process next re-enters the kernel.
  p->trapframe->kernel_satp = r_satp();         // kernel page table
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp();         // hartid for cpuid()

  // set up the registers that trampoline.S's sret will use
  // to get to user space.
  
  // set S Previous Privilege mode to User.
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE; // enable interrupts in user mode
  w_sstatus(x);

  // set S Exception Program Counter to the saved user pc.
  w_sepc(p->trapframe->epc);

  // tell trampoline.S the user page table to switch to.
  uint64 satp = MAKE_SATP(p->pagetable);

  // jump to trampoline.S at the top of memory, which 
  // switches to the user page table, restores user registers,
  // and switches to user mode with sret.
  uint64 fn = TRAMPOLINE + (userret - trampoline);
  ((void (*)(uint64,uint64))fn)(TRAPFRAME, satp);
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void 
kerneltrap()
{
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();
  
  if((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if(intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  // Check for device interrupt
  if((which_dev = devintr()) == 0) {
    // Check for page-fault exception (scause value 12)
    if (scause == 12) {
      printf("Page-fault exception: sepc=%p stval=%p\n", r_sepc(), r_stval());
      // Handle the page-fault exception here
      
    } else {
      // Print information about the unexpected scause
      printf("Unexpected scause: %p sepc=%p stval=%p\n", scause, r_sepc(), r_stval());
      panic("kerneltrap");
    }
  }

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2 && myproc() != 0 && myproc()->state == RUNNING)
    yield();

  // the yield() may have caused some traps to occur,
  // so restore trap registers for use by kernelvec.S's sepc instruction.
  w_sepc(sepc);
  w_sstatus(sstatus);
}

void
clockintr()
{
  acquire(&tickslock);
  ticks++;
  wakeup(&ticks);
  release(&tickslock);
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int
devintr()
{
  uint64 scause = r_scause();

  if((scause & 0x8000000000000000L) &&
     (scause & 0xff) == 9){
    // this is a supervisor external interrupt, via PLIC.

    // irq indicates which device interrupted.
    int irq = plic_claim();

    if(irq == UART0_IRQ){
      uartintr();
    } else if(irq == VIRTIO0_IRQ){
      virtio_disk_intr();
    } else if(irq){
      printf("unexpected interrupt irq=%d\n", irq);
    }

    // the PLIC allows each device to raise at most one
    // interrupt at a time; tell the PLIC the device is
    // now allowed to interrupt again.
    if(irq)
      plic_complete(irq);

    return 1;
  } else if(scause == 0x8000000000000001L){
    // software interrupt from a machine-mode timer interrupt,
    // forwarded by timervec in kernelvec.S.

    if(cpuid() == 0){
      clockintr();
    }
    
    // acknowledge the software interrupt by clearing
    // the SSIP bit in sip.
    w_sip(r_sip() & ~2);

    return 2;
  } else {
    return 0;
  }
}

