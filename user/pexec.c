#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/pstat.h"
#include "kernel/param.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <priority> <command> [args...]\n", argv[0]);
        return 1;
    }

    int priority = atoi(argv[1]);
    if (priority < 0 || priority > 39) {
        fprintf(stderr, "Priority must be in the range 0-39.\n");
        return 1;
    }

    // Shift the priority value to create a bitmask.
    unsigned int priority_mask = (1 << (31 - priority));

    // Set the priority mask using system call (adjust as per your OS).
    if (sched_setscheduler(0, SCHED_RR, &priority_mask) == -1) {
        perror("sched_setscheduler");
        return 1;
    }

    // Execute the specified command and arguments.
    execvp(argv[2], &argv[2]);

    // If execvp fails, print an error message.
    perror("execvp");
    return 1;
}

