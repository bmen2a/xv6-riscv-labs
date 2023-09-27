
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/pstat.h"

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
        struct pstat *stat;
        
        // Allocate memory for stat
        
        stat = (struct pstat *)malloc(sizeof(struct pstat));

        if (stat == NULL) {
            printf("Failed to allocate memory for stat\n");
            exit(1);
        }
        
        //// Initialize stat structure
        memset(&stat, 0, sizeof(struct pstat));
        
        wait2(&stat, &status);
	
	     

        // Get the current time after child process has finished
        int end_time = uptime();
         // Calculate elapsed time
        int elapsed_time = end_time - start_time;   
        
	int cpu_time = stat.cutime[pid] + stat.cstime[pid];
        int cpu_usage = (cpu_time * 100) / elapsed_time;
	
        // Calculate and print the time difference
        printf( "Time taken: %d ticks\n", elapsed_time);
       printf("elapsed time: %d ticks, cpu time: %d ticks, %d%% CPU\n", elapsed_time, cpu_time, cpu_usage);
       
       // Free the allocated memory
       free(stat);

    }
    exit(0);
}
