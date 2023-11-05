#include "loader.h" // Include necessary header file

// Global variables
Elf32_Ehdr *ehdr; // Pointer to the struct ELF header
Elf32_Phdr *phdr; // Pointer to the struct ELF program header
int fd; // File descriptor
int numPageFaults = 0; // Counter for page faults
int numPageAllocations = 0; // Counter for page allocations
long long internalFragmentation = 0; // Counter for internal fragmentation

// Function to clean up allocated memory
void loader_cleanup() {
    if (ehdr != NULL) { // Check if the memory allocated to the ehdr variable has been freed
        free(ehdr);
        ehdr = NULL;
    }

    if (phdr != NULL) { // Check if the memory allocated to the phdr variable has been freed
        free(phdr);
        phdr = NULL;
    }
}

// Signal handler for the segmentation fault
void sigsegv_handler(int signal, siginfo_t *info, void *context) {
    numPageFaults++; // Increment the page fault counter

    // Get the address where the segmentation fault occurred
    unsigned long segfault_address = (unsigned long)info->si_addr;

    // Allocate memory for the program header
    phdr = (Elf32_Phdr *)malloc(sizeof(Elf32_Phdr));
    if (!phdr) {
        perror("Failed to allocate memory for phdr");
        exit(EXIT_FAILURE);
    }

    int i = 0;

    // Iterate through the program headers to find the corresponding memory segment
    while (i < ehdr->e_phnum) {
        ssize_t bytes_r = read(fd, phdr, sizeof(Elf32_Phdr)); // Read the program header from the file

        if (bytes_r < 0) {
            perror("Failed to read the file to phdr");
            close(fd);
            exit(EXIT_FAILURE);
        }

        // Check if the segment contains the address where the segmentation fault occurred
        if (phdr->p_type == PT_LOAD && segfault_address <= phdr->p_vaddr + phdr->p_memsz && segfault_address >= phdr->p_vaddr) {
            // Calculate the number of pages required for the segment
            int segmentSize = phdr->p_memsz;
            int numPages = (segmentSize - 1) / 4096 + 1;

            // Map each page of the segment to the virtual memory
            for (size_t j = 0; j < numPages; j++) {
                size_t offset_4096 = j * 4096;
                size_t remaining = segmentSize - offset_4096;
                size_t map_size = remaining < 4096 ? remaining : 4096;

                // Map the memory
                void *virtual_mem = mmap((void *)(segfault_address + offset_4096), 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE, fd, phdr->p_offset);
                if (virtual_mem == MAP_FAILED) {
                    perror("mmap failed");
                    close(fd);
                    exit(EXIT_FAILURE);
                }

                // Update counters
                numPageAllocations += 1;
                internalFragmentation += 4096 - map_size;
            }

            // Reset the file offset to the program header table
            if (lseek(fd, ehdr->e_phoff, SEEK_SET) == -1) {
                perror("Error seeking file");
                close(fd);
                exit(EXIT_FAILURE);
            }
            break;
        }
        i++;
    }
}

// Function to load and run the ELF file
void load_and_run_elf(char **exe) {
    // Open the ELF file
    fd = open(exe[1], O_RDONLY);
    if (fd < 0) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    // Allocate memory for the ELF header
    ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
    if (!ehdr) {
        perror("Failed to allocate memory for ehdr");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Read the ELF header from the file
    ssize_t bytes = read(fd, ehdr, sizeof(Elf32_Ehdr));
    if (bytes < 0) {
        perror("Failed to read the file(ehdr)");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Get the entry point of the ELF file
    Elf32_Addr counter = ehdr->e_entry;

    // Set up the signal handler for segmentation faults
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = sigsegv_handler;

    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("Error with sigaction sigsegv");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Call the entry point of the ELF file
    int (*_start)() = (int (*)())(counter);
    int result = _start();

    // Print the results
    printf("User _start return value = %d\n", result);
    printf("No of page faults: %d\n", numPageFaults);
    printf("No of page allocations: %d\n", numPageAllocations);
    printf("Internal fragmentation (in bytes): %lld\n", internalFragmentation);

    // Close the ELF file
    if (close(fd) == -1) {
        perror("Error closing file");
        exit(EXIT_FAILURE);
    }
}

// Main function to execute the program
int main(int argc, char **argv) {
    // Check for the correct number of arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <ELF Executable>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Load and run the ELF file
    load_and_run_elf(argv);

    // Check if loading and running the ELF file failed
    if (ehdr == NULL || phdr == NULL) {
        fprintf(stderr, "Error: Unable to load and run the ELF file.\n");
        exit(EXIT_FAILURE);
    }

    // Clean up allocated memory
    loader_cleanup();

    // Exit the program with a success status
    return 0;
}
