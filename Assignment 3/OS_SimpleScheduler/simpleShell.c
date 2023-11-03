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
#include "common.h"
#include <sys/mman.h>
#include <signal.h>

#define MAX_INPUT_SIZE 1000
#define MAX_HISTORY_SIZE 500

int ready = 0;

char *history[MAX_HISTORY_SIZE]; // Array to store command history
int history_count = 0; // Counter for the number of commands in history
pid_t process_pid[MAX_HISTORY_SIZE]; // Array to store process IDs
int pid_count = 0; // Counter for the number of process IDs
time_t start_time[MAX_HISTORY_SIZE]; // Array to store start times of processes
int start_count = 0; // Counter for the number of start times
double execution_time[MAX_HISTORY_SIZE]; // Array to store execution times of processes
int exec_time = 0; // Counter for the number of execution times

// Function to add a command to the history
void addToHistory(char *command) {
    if (history_count < MAX_HISTORY_SIZE) {
        history[history_count++] = strdup(command);
    } 
    else {
        free(history[0]);
        for (int i = 0; i < MAX_HISTORY_SIZE - 1; i++) {
            history[i] = history[i + 1];
        }
        history[MAX_HISTORY_SIZE - 1] = strdup(command);
    }
}

// Function to parse the input command into parameters
void parseInput(char *input, char *parameters[], int *counter) {
    *counter = 0;
    char *token = strtok(input, " ");

    while (token != NULL) {
        parameters[(*counter)] = token;
        (*counter)++;

        token = strtok(NULL, " ");
    }
}

// Function to launch a new process
int launch(char *command, char *args[]) {
    int pid;
    time_t start, end;

    pid = fork();

    if (pid < 0) {
        perror("Forking error");
    } 
    else if (pid == 0) {
        if (execvp(command, args) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } 
    else {
        process_pid[pid_count++] = pid;
        time(&start);

        waitpid(pid, NULL, 0);

        time(&end);
        execution_time[exec_time++] = difftime(end, start);
        start_time[start_count++] = start; 
    }

    return 0;
}

int main(int argc, char *argv[]) {
    // Set up shared memory
    shm_t *shared_memory = setup();
    strcpy(shared_memory->file_name,"");
    for (int i = 0; i < 128;i++){
        shared_memory->running[i] = false;
    }
    shared_memory->counter = 0;

    // Fork a new process
    int pid = fork();

    if (pid < 0){
        printf("There is some error with forking\n");
    }
    else if(pid == 0){
        // Execute the scheduler program
        char *args[4] = {"./sch",argv[1], argv[2], NULL};
        execvp(args[0],args);

        perror("Error in exec function");
        exit(0);
    }
    
    char input[MAX_INPUT_SIZE];
    char *parameters[MAX_INPUT_SIZE];
    int counter;

    // Main shell loop
    while (true) {
        bool flag = false;
        int count = 0;

        printf("shash@DESKTOP-4FHN094:~/Group-46$ ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            // Print command history when the shell is closed
            for (int i = 0; i < history_count;i++){
                printf("%d:%s\n",i+1,history[i]);
                printf("PID: %d\n",process_pid[i]);
                printf("Start time: %s\n",ctime(&start_time[i]));
                printf("Execution time: %.2f s\n",execution_time[i]);
            }
            break;
        }

        // Check for the presence of the pipe character in the input
        for (int i = 0; input[i] != '\0';i++){
            if (input[i] == '|'){
                flag= true;
                count++;
            }
        }

        input[strcspn(input, "\n")] = '\0';

        // Handle different input commands
        if (strcmp(input,"exit") == 0){
            // Print command history when the shell is exited
            for (int i = 0; i < history_count;i++){
                printf("%d:%s\n",i+1,history[i]);
                printf("PID: %d\n",process_pid[i]);
                printf("Start time: %s\n",ctime(&start_time[i]));
                printf("Execution time: %.2f s\n",execution_time[i]);
            }
            break;
        }
        char input_copy[MAX_INPUT_SIZE];
        strcpy(input_copy,input);

        char* token = strtok(input_copy, " ");
        if (strcmp(token,"submit") == 0){
            // Parse the submit command and update shared memory
            token= strtok(NULL, " ");
            char file_name[MAX_INPUT_SIZE];
            strcpy(file_name, token);
            int priority = 1;

            token = strtok(NULL, " ");

            if (token != NULL){
                priority = atoi(token);
            }
            
            strcat(shared_memory->file_name,file_name);
            strcat(shared_memory->file_name,"|");
            
            shared_memory->running[shared_memory->counter++] = true; 
        }
        else if (strcmp(input, "history") == 0) {
            // Print the command history
            for (int i = 0; i < history_count; i++) {
                printf("%d: %s\n", i + 1, history[i]);
            }
        }
        else {
            // Add the command to history, parse input, and launch the process
            addToHistory(input);
            parseInput(input, parameters,&counter);
            parameters[counter++] = NULL;
            launch(parameters[0],parameters);
        }
    }

    // Free dynamically allocated memory and wait for child process to finish
    for (int i = 0; i < history_count; i++) {
        free(history[i]);
    }
    wait(NULL); 

    // Clean up shared memory
    cleanup(shared_memory);

    return 0;
}
