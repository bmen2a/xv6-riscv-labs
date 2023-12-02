// semaphore.c
#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "spinlock.h"


struct semtab semtable;

void seminit(void) {
    initlock(&semtable.lock, "semtable");

    for (int i = 0; i < NSEM; i++)
        initlock(&semtable.sem[i].lock, "sem");
}

// Allocate an unused location in the semaphore table
// Returns the index of the allocated semaphore or -1 if there is no empty location
int semalloc(void) {
    acquire(&semtable.lock);

    for (int i = 0; i < NSEM; i++) {
        if (!holding(&semtable.sem[i].lock)) {
            initlock(&semtable.sem[i].lock, "sem");
            release(&semtable.lock);
            return i;
        }
    }

    release(&semtable.lock);
    return -1;  // No empty location
}

// Deallocate a semaphore entry in the semaphore table
void semdealloc(int index) {
    if (index < 0 || index >= NSEM)
        panic("semdealloc: invalid semaphore index");

    acquire(&semtable.lock);

    if (holding(&semtable.sem[index].lock)) {
        release(&semtable.sem[index].lock);
        initlock(&semtable.sem[index].lock, "sem");
    }

    release(&semtable.lock);
}

