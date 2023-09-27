
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]){
 if (argc < 2) {
        printf( "Usage: time <command>\n");
        exit(1);
    }

    // Get the current time before forking
    int start_time = uptime();

    int pid = fork();
    if (pid < 0) {
        printf( "Fork failed\n");
        exit(1);
    }
    if (pid == 0) {
        // This is the child process
        exec(argv[1], argv + 1);
        printf( "Execution failed\n");
        exit(1);
    } else {
        // This is the parent process
        int status;
        wait(&status);

        // Get the current time after child process has finished
        int end_time = uptime();

        // Calculate and print the time difference
        printf( "Time taken: %d ticks\n", end_time - start_time);
    }
    exit(0);
}
