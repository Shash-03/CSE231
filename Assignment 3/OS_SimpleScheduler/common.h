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

#define MAX_INPUT_SIZE 1000
#define MAX_HISTORY_SIZE 500
#define SHM_NAME "/shared_memory"

typedef struct shm_t{
    char file_name[MAX_INPUT_SIZE];
    bool running[128];
    int counter;
    int priority[128];
}shm_t;

void* setup() {
    void *ptr = MAP_FAILED; 

    
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Opening error\n");
        return ptr;
    }

    
    if (ftruncate(shm_fd, 9192) == -1) {
        perror("Truncating error\n");
        close(shm_fd);
        return ptr;
    }

    
    ptr = mmap(NULL, 9192, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("Mmap error\n");
        close(shm_fd);
        return ptr;
    }

    return ptr;
}


void cleanup(void *ptr) {
    if (munmap(ptr, 9192) == -1) {
        perror("Unmapping shared memory");
        
    }
    if (shm_unlink(SHM_NAME) == -1) {
        perror("Unlinking error");
    }
}