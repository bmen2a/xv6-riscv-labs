#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]){
int count=0;
count=atoi(argv[1]);
sleep(count);
printf("[%d] clock ticks\n", count);
exit(0);
}
