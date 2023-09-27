#include <stdio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]){
 if (argc < 2) {
        printf(2, "Usage: time <command>\n");
        exit();
    }

    // Get the current time before forking
    uint start_time = uptime();

    int pid = fork();
    if (pid < 0) {
        printf(2, "Fork failed\n");
        exit();
    }
    if (pid == 0) {
        // This is the child process
        exec(argv[1], argv + 1);
        printf(2, "Execution failed\n");
        exit();
    } else {
        // This is the parent process
        int status;
        wait(&status);

        // Get the current time after child process has finished
        uint end_time = uptime();

        // Calculate and print the time difference
        printf(1, "Time taken: %d ticks\n", end_time - start_time);
    }
    exit();
 
}
