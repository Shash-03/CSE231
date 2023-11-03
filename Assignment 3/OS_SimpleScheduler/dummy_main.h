#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/mman.h>

void sigintHandler(int sig_num) {
}
int dummy_main(int argc, char *argv[]);
int main(int argc, char *argv[]) {
    signal(SIGINT, sigintHandler);
    int ret = dummy_main(argc, argv);
    return ret;
}
#define main dummy_main