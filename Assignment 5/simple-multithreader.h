#include <iostream>
#include <list>
#include <functional>
#include <stdlib.h>
#include <cstring>
#include <chrono>

// Struct for thread arguments in a 1D loop
typedef struct {
    int low;
    int high;
    std::function<void(int)> lambda;
} thread_args;

// Struct for thread arguments in a 2D loop
typedef struct {
    int low1;
    int high1;
    int low2;
    int high2;
    std::function<void(int, int)> lambda;
} thread_args_matrix;

// Thread function for a 1D loop
void* thread_func(void* ptr) {
    thread_args* args = static_cast<thread_args*>(ptr);
    for (int i = args->low; i < args->high; ++i) {
        args->lambda(i);
    }
    return NULL;
}

// Thread function for a 2D loop
void* thread_func_matrix(void* ptr) {
    thread_args_matrix* args = static_cast<thread_args_matrix*>(ptr);
    for (int i = args->low1; i < args->high1; ++i) {
        for (int j = args->low2; j < args->high2; ++j) {
            args->lambda(i, j);
        }
    }
    return NULL;
}

// Parallel for loop for a 1D range
void parallel_for(int low, int high, std::function<void(int)> lambda, int numThreads) {
    numThreads--;

    // Record start time
    auto start = std::chrono::high_resolution_clock::now();

    // Thread management
    pthread_t tid[numThreads];
    thread_args args[numThreads];
    int chunk = (high - low) / numThreads;

    int counter = numThreads;

    if ((high - low) % numThreads != 0) {
        counter--;
    }

    for (int i = 0; i < counter; i++) {
        args[i].low = i * chunk;
        args[i].high = (i + 1) * chunk;
        args[i].lambda = lambda;
        pthread_create(&tid[i], NULL, thread_func, static_cast<void*>(&args[i]));
    }

    pthread_t residue;
    thread_args data;

    if (counter != numThreads) {
        data.high = high;
        data.low = counter * chunk;
        data.lambda = lambda;
        pthread_create(&residue, NULL, thread_func, static_cast<void*>(&data));
    }

    // Join threads
    for (int i = 0; i < counter; i++) {
        pthread_join(tid[i], NULL);
    }

    if (counter != numThreads) {
        pthread_join(residue, NULL);
    }

    // Record end time
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    // Print execution time
    std::cout << "Execution time: " << duration.count() << " seconds" << std::endl;
}

// Parallel for loop for a 2D range
void parallel_for(int low1, int high1, int low2, int high2,
    std::function<void(int, int)> lambda, int numThreads) {
    numThreads--;

    // Record start time
    auto start = std::chrono::high_resolution_clock::now();

    // Thread management
    pthread_t tid[numThreads];
    thread_args_matrix args[numThreads];
    int chunk1 = (high1 - low1) / numThreads;
    int chunk2 = (high2 - low2) / numThreads;

    int counter = numThreads;

    if ((high1 - low1) % numThreads != 0) {
        counter--;
    }

    for (int i = 0; i < counter; i++) {
        args[i].low1 = i * chunk1;
        args[i].high1 = (i + 1) * chunk1;
        args[i].low2 = low2;
        args[i].high2 = high2;
        args[i].lambda = lambda;
        pthread_create(&tid[i], NULL, thread_func_matrix, static_cast<void*>(&args[i]));
    }

    pthread_t residue;
    thread_args_matrix data;

    if (counter != numThreads) {
        data.high1 = high1;
        data.low1 = counter * chunk1;
        data.low2 = low2;
        data.high2 = high2;
        data.lambda = lambda;

        pthread_create(&residue, NULL, thread_func_matrix, static_cast<void*>(&data));
    }

    // Join threads
    for (int i = 0; i < counter; ++i) {
        pthread_join(tid[i], NULL);
    }

    if (counter != numThreads) {
        pthread_join(residue, NULL);
    }

    // Record end time
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    // Print execution time
    std::cout << "Execution time: " << duration.count() << " seconds" << std::endl;
}

// Main function provided by the user
int user_main(int argc, char **argv);

// Entry point
int main(int argc, char **argv) {
    // Call user's main function
    int rc = user_main(argc, argv);

    // Return the user's main function result
    return rc;
}

// Redefine 'main' to be 'user_main'
#define main user_main
