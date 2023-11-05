#include "loader.h"

Elf32_Ehdr *ehdr;     //Declares a pointer to the struct ELF header
Elf32_Phdr *phdr;     //Declares a pointer to the struct ELF program header
int fd;               //Declares an integer to store the value of the file descriptor
int numPageFaults = 0;
int numPageAllocations = 0;
long long internalFragmentation = 0;


void loader_cleanup() {

  if (ehdr != NULL){ //After running the code, we are checking if the memory allocated to the variables has been freed up
    free(ehdr);
    ehdr = NULL;
  }

  if(phdr != NULL){
    free(phdr);
    phdr = NULL;
  }
}




void sigsegv_handler(int signal, siginfo_t *info, void *context) {
    numPageFaults++;

    unsigned long segfault_address = (unsigned long)info->si_addr;
    phdr = (Elf32_Phdr*)malloc(sizeof(Elf32_Phdr)); // Allocates memory to the struct
    int i = 0;

    while (i < ehdr->e_phnum) { 
        ssize_t bytes_r = read(fd, phdr, sizeof(Elf32_Phdr)); 

        if (bytes_r < 0) { 
            perror("Failed to read the file(phdr)");
            close(fd);
            free(phdr); 
            return;
        }

        if (phdr->p_type == PT_LOAD) { 
            if (segfault_address <= phdr->p_vaddr + phdr->p_memsz && segfault_address >= phdr->p_vaddr) { 
                int segment_size = phdr->p_memsz; 
                int num_pages = (segment_size - 1) / 4096 + 1; 

                for (size_t i = 0; i < num_pages; i++) {
                    size_t offset_4096 = i * 4096;
                    size_t remaining = segment_size - offset_4096;
                    size_t map_size = remaining < 4096 ? remaining : 4096;

                    
                    void* virtual_mem = mmap((void*)(segfault_address + offset_4096), 4096, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE, fd, phdr->p_offset);
                    if (virtual_mem == MAP_FAILED) {
                        perror("mmap failed");
                        break;
                    }

                    
                    numPageAllocations += 1;
                    internalFragmentation += 4096 - map_size;
                }

                lseek(fd, ehdr->e_phoff, SEEK_SET);
                break;
            }
        }
        i++;
        
    }

    
}


void load_and_run_elf(char** exe) {
  fd = open(exe[1], O_RDONLY);   //Stores the content of the file in the file descriptor
  
  if (fd < 0){                   //Throws an error if the value of the file descriptor is -1
    perror("Failed to open file");
    return;
  }

  else{
    ehdr = (Elf32_Ehdr*)malloc(sizeof(Elf32_Ehdr)); 
    ssize_t bytes = read(fd,ehdr,sizeof(Elf32_Ehdr));
    if(bytes  < 0){ 
      perror("Failed to read the file(ehdr)");
      close(fd);
      return;
    }
    Elf32_Addr counter = ehdr->e_entry; 
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = sigsegv_handler;

    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("Error with sigaction sigsev");
        exit(EXIT_FAILURE);
    }

    int (*_start)() = (int (*)())(counter);

    int result = _start();

    printf("User _start return value = %d\n",result);

    printf("No of page faults: %d\n", numPageFaults);
    printf("No of page allocations: %d\n", numPageAllocations);
    printf("Internal fragmentation (in btyes): %lld\n",internalFragmentation);
    


    close(fd);
  }
}

int main(int argc, char** argv) 
{
  if(argc != 2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }
  load_and_run_elf(argv);
  loader_cleanup();
  return 0;
}