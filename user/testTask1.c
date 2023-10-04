#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/pstat.h"
#include "kernel/param.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(2, "Usage: %s <new_priority>\n", argv[0]);
        exit(1);
    }

    int newPriority = atoi(argv[1]);

    // Call the sys_setpriority system call
    if (setpriority(newPriority) < 0) {
        fprintf(2, "Failed to set priority.\n");
        exit(1);
    }

    printf("Priority set to: %d\n", newPriority);

    // Run your modified ps command to check the priority
    if (exec("/user/ps", argv) < 0) {
        fprintf(2, "Error running ps command.\n");
        exit(1);
    }

    exit(0);
}
