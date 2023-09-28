#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/pstat.h"
#include "kernel/param.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: time <command>\n");
        exit(1);
    }

    // Get the current time before forking
    int start_time = uptime();

    int pid = fork();
    if (pid < 0) {
        printf("Fork failed\n");
        exit(1);
    }
    if (pid == 0) {
        // This is the child process
        exec(argv[1], argv + 1);
        printf("Execution failed\n");
        exit(1);
    } else {
        // This is the parent process
        int status;
        struct rusage rusage; // Define rusage for wait2

        // Call wait2 with proper arguments
        wait2(&status, &rusage);

        // Get the current time after child process has finished
        int end_time = uptime();
        // Calculate elapsed time
        int elapsed_time = end_time - start_time;

        int cpu_time = rusage.cputime;
        int cpu_usage = (cpu_time / elapsed_time) * 100;

        // Calculate and print the time difference
        printf("Time taken: %d ticks\n", elapsed_time);
        printf("Elapsed time: %d ticks, cpu time: %d ticks, %d% CPU\n", elapsed_time, cpu_time, cpu_usage);
    }
    exit(0);
}

