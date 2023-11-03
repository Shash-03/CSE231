#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include "common.h"

#define MAX_QUEUE_SIZE 128

// Process structure to represent a process
typedef struct Process{
    int pid;
    char name[1000];
    struct timespec start_time;
    long long exec_time;
    int no_cycles;
    long long wait_time;
} Process;

// Queue structure to implement a queue for processes
typedef struct {
    Process items[MAX_QUEUE_SIZE];
    int front, rear;
} Queue;

// Function to initialize the queue
void initializeQueue(Queue *q) {
    q->front = -1;
    q->rear = -1;
}

// Function to check if the queue is empty
bool isEmpty(Queue *q) {
    return q->front == -1;
}

// Function to check if the queue is full
bool isFull(Queue *q) {
    return (q->rear + 1) % MAX_QUEUE_SIZE == q->front;
}

// Function to enqueue a process into the queue
void enqueue(Queue *q, Process item) {
    if (isFull(q)) {
        printf("Queue is full. Cannot enqueue any more items.\n");
    } else {
        if (isEmpty(q)) {
            q->front = 0;
        }
        q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
        q->items[q->rear] = item;
    }
}

// Function to dequeue a process from the queue
Process dequeue(Queue *q) {
    Process item;
    if (isEmpty(q)) {
        printf("Queue is empty. Cannot dequeue any items.\n");
        item.pid = -1; 
    } else {
        item = q->items[q->front];
        if (q->front == q->rear) {
            initializeQueue(q); 
        } else {
            q->front = (q->front + 1) % MAX_QUEUE_SIZE;
        }
    }
    return item;
}

// Function to calculate the time difference between two timespec structures
long long timespec_diff(struct timespec start, struct timespec end) {
    long long diff = (end.tv_sec - start.tv_sec) * 1000LL; 
    diff += (end.tv_nsec - start.tv_nsec) / 1000000LL; 
    return diff;
}

// Array to store the history of processes
Process history[128] = {0};
int history_counter = 0;

// Signal handler for SIGINT
void sigintHandler(int sig_num) {
    int counter = 1;
    for (int i = 0; i < 128 ; i++){
        if (history[i].pid != 0){
            printf("%d.\n", counter);
            counter++;
            printf("File name:%s\n",history[i].name);
            printf("PID:%d\n",history[i].pid);
            printf("Execution time: %lld\n",history[i].exec_time);
            printf("Waiting time: %lld\n",history[i].wait_time);
        }
    }
}

int main(int argc, char *argv[]){    
    shm_t *shared_memory = setup();
    
    signal(SIGINT, sigintHandler);

    int num_cpu = atoi(argv[1]);
    int tslice = atoi(argv[2]);
    
    Queue *q = (Queue*)malloc(sizeof(Queue));
    
    initializeQueue(q);

    Process executing_process[128] = {0};
    bool is_executing[128];
    int is_waiting = 0;
    int length = 0;

    while(true){
        // Check if there are any processes in the system
        bool flag = false;
        for (int i =0; i < 128;i++){
            flag = flag || shared_memory->running[i];
        }

        for (int i = 0 ; i < length ;i++){
            flag = flag || is_executing[i];
        }

        flag = flag || is_waiting > 0;

        // If there are no processes, sleep for a duration equal to the time slice
        if (!flag){
            usleep(tslice * 1000);
        }

        // If there are processes, perform the required actions
        else{
            // Extract the file names from the shared memory
            char *tokens[128]; 
            char input[1000];
            strcpy(input,shared_memory->file_name);
            char *token = strtok(input, "|"); 
            int count = 0;
            while (token != NULL && count < 128) {
                tokens[count] = token;
                token = strtok(NULL, "|"); 
                count++;
            }

            // Enqueue user processes from the shared memory into the queue
            for (int i = 0; i < 128;i++){
                if (shared_memory->running[i]){
                    printf("User process enqueued\n");
                    Process p = {0};
                    strcpy(p.name, tokens[i] );
                    enqueue(q, p);
                    struct timespec start;
                    clock_gettime(CLOCK_MONOTONIC,&start);
                    history[history_counter++] = p;
                    history[history_counter - 1].start_time = start;
                    history[history_counter - 1].no_cycles = 1;
                    shared_memory -> running[i] = false;
                    is_waiting++;
                }
            }

            // Enqueue currently executing processes into the queue
            for (int i = 0; i < length;i++){
                if (is_executing[i]){
                    printf("Running process enqueued\n");
                    enqueue(q,executing_process[i]);
                }
            }

            // Reset the state of the is_executing array and length
            for (int i = 0; i < 128;i++){
                is_executing[i] = false;
            }
            length = 0;

            // Dequeue processes from the queue and execute them
            for (int i = 0; i < num_cpu && !isEmpty(q);i++){
                printf("User process dequeued\n");
                is_waiting--;
                Process p = dequeue(q);
                is_executing[length] = true;
                executing_process[length++] = p;

                if (p.pid == 0){
                    // Fork new processes and execute them
                    int pid = fork();
                    executing_process[length - 1].pid = pid;
                    
                    if (pid < 0){
                        perror("Error in forking\n");
                    }
                    else if (pid == 0){
                        char *args[2] = {p.name,NULL};
                        printf("Process starts executing\n");
                        execvp(args[0],args);
                        perror("execvp");
                    }
                }
                else{
                    // Handle already running processes
                    for (int j = 0; j < 128;j++){
                        if (strcmp(p.name, history[j].name)==0){
                            history[j].no_cycles++;
                        }
                    }
                    printf("Process starts executing\n");
                    kill(p.pid, SIGCONT);
                }
            }
            
            // Sleep for a duration equal to the time slice
            usleep(tslice * 1000);
            
            // Stop the currently executing processes
            for (int i = 0; i < length ;i++){
                kill(executing_process[i].pid,SIGSTOP);
            }
            
            // Check the status of the executing processes and update the history
            for (int i = 0; i < length;i++){
                int status;
                if (waitpid(executing_process[i].pid,&status, WNOHANG)!= 0){
                    printf("Process terminated\n");
                    struct timespec end;
                    clock_gettime(CLOCK_MONOTONIC,&end);
                    for (int j = 0; j < 128;j++){
                        if (strcmp(executing_process[i].name,history[j].name) == 0){
                            history[j].pid = executing_process[i].pid;
                            history[j].exec_time = timespec_diff(executing_process[j].start_time,end);
                            history[j].wait_time = history[j].exec_time - (long long)history[j].no_cycles*(long long)tslice;
                        }
                    }
                    is_executing[i] = false;
                }
            }
        }
    }

    // Clean up the shared memory
    cleanup(shared_memory);

    return 0;
}
