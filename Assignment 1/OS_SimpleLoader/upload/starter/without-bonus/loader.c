#include "loader.h"

Elf32_Ehdr *ehdr;     //Declares a pointer to the struct ELF header
Elf32_Phdr *phdr;     //Declares a pointer to the struct ELF program header
int fd;               //Declares an integer to store the value of the file descriptor

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

void load_and_run_elf(char** exe) {
  fd = open(exe[1], O_RDONLY);   //Stores the content of the file in the file descriptor
  
  if (fd < 0){                   //Throws an error if the value of the file descriptor is -1
    perror("Failed to open file");
    return;
  }

  else{
    ehdr = (Elf32_Ehdr*)malloc(sizeof(Elf32_Ehdr)); //Allocates memory to the struct 
    ssize_t bytes = read(fd,ehdr,sizeof(Elf32_Ehdr));//Reads the relevant data from the file descriptor into the memory address pointed towards by the pointer ehdr
    if(bytes  < 0){ //Throws an error if the value of the read function is invalid
      perror("Failed to read the file(ehdr)");
      close(fd);
      return;
    }
    Elf32_Addr counter = ehdr->e_entry; //Storing the entry point address into a variable called counter
    phdr = (Elf32_Phdr*)malloc(sizeof(Elf32_Phdr)); //Allocates memory to the struct
    int i = 0;
    while(i < ehdr ->e_phnum){ //Runs a while loop to iterate through the program headers
      ssize_t bytes_r = read(fd,phdr,sizeof(Elf32_Phdr));//Reads the relevant data from the file descriptor into the memory address pointed towards by the pointer phdr
      if (bytes_r < 0){ //Throws an error if the value of the read function is invalid
        perror("Failed to read the file(phdr)");
        close(fd);
        return;
      }
      if (phdr->p_type == PT_LOAD){ //Checks if the p_type attribute of the program header is equal to PT_LOAD
        if (counter <= phdr -> p_vaddr + phdr -> p_memsz && counter >= phdr ->p_vaddr ){//Checks if the entry point address lies in the segment with p_type attribute PT_LOAD
          break;
        }
      }
      i++;
    }

    void* virtual_mem = mmap(NULL,phdr->p_memsz,PROT_READ|PROT_WRITE|PROT_EXEC ,MAP_PRIVATE ,fd,phdr->p_offset); //Declare a pointer pointing towards a memory mapping
//from offset equivalent to the value of p_offset with the size of p_memsz
    if(virtual_mem == MAP_FAILED){//Throws an error if the mapping failed
      perror("Failed to map ");
      close(fd);
      return;
    }
    
    int (*_start)() = (int (*)())(virtual_mem + (counter - phdr->p_vaddr));//Typecasting the required memory address into the same type as _start
    int result = _start();

    printf("User _start return value = %d\n",result);


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