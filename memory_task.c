#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    #define PAGE_SIZE 4096
#else
    #include <unistd.h>
    #include <sys/mman.h>
    #define PAGE_SIZE 4096
#endif

#define BLOCK_SIZE (128 * 1024 * 1024)

typedef struct {
    void* address;
    int is_filled;
    #ifdef _WIN32
        DWORD permissions;
    #else
        int permissions;
    #endif
} MemoryBlock;

MemoryBlock blocks[10] = {0};
int block_count = 0;
size_t virtual_memory = 0;
size_t physical_memory = 0;

void clear_input() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void* platform_alloc(size_t size) {
    #ifdef _WIN32
        // Windows: VirtualAlloc
        return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    #else
        // Linux: mmap
        return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    #endif
}

int platform_free(void* addr, size_t size) {
    #ifdef _WIN32
        // Windows: VirtualFree
        return VirtualFree(addr, 0, MEM_RELEASE);
    #else
        // Linux: munmap
        return munmap(addr, size) == 0;
    #endif
}

#ifdef _WIN32
    #define GET_READ_WRITE_PERMISSIONS() PAGE_READWRITE
    #define HAS_WRITE_PERMISSION(perms) ((perms) == PAGE_READWRITE)
#else
    #define GET_READ_WRITE_PERMISSIONS() (PROT_READ | PROT_WRITE)
    #define HAS_WRITE_PERMISSION(perms) ((perms) & PROT_WRITE)
#endif

int main() {
    int choice;
    
    printf("Memory Management Demo (Cross-platform)\n");
    printf("Platform: ");
    #ifdef _WIN32
        printf("Windows\n");
        printf("Process ID: %lu\n", GetCurrentProcessId());
    #else
        printf("Linux\n");
        printf("Process ID: %d\n", getpid());
    #endif
    printf("Page size: %d bytes\n", PAGE_SIZE);
    printf("Block size: %d MB\n\n", BLOCK_SIZE / (1024 * 1024));
    
    while (1) {
        printf("Menu:\n");
        printf("1. Allocate virtual memory (read)\n");
        printf("2. Initialize physical memory (write)\n");
        printf("3. Free last block\n");
        printf("4. Exit\n");
        printf("Choice: ");
        
        if (scanf("%d", &choice) != 1) {
            printf("Error: Invalid input\n");
            clear_input();
            continue;
        }
        clear_input();
        
        if (choice == 1) {
            if (block_count >= 10) {
                printf("Error: Maximum 10 blocks reached\n");
                continue;
            }
            
            printf("Allocating %d MB of virtual memory...\n", 
                   BLOCK_SIZE / (1024 * 1024));
            
            void* mem = platform_alloc(BLOCK_SIZE);
            
            #ifndef _WIN32
                if (mem == MAP_FAILED) {
                    mem = NULL;
                }
            #endif
            
            if (mem != NULL) {
                blocks[block_count].address = mem;
                blocks[block_count].is_filled = 0;
                blocks[block_count].permissions = GET_READ_WRITE_PERMISSIONS();
                
                virtual_memory += BLOCK_SIZE;
                block_count++;
                
                printf("Success!\n");
                printf("Virtual memory: %.1f MB\n", 
                       virtual_memory / (1024.0 * 1024));
                printf("Physical memory: %.1f MB\n", 
                       physical_memory / (1024.0 * 1024));
                printf("Blocks allocated: %d\n", block_count);
            } else {
                printf("Allocation failed!\n");
                #ifdef _WIN32
                    printf("Error code: %lu\n", GetLastError());
                #else
                    perror("mmap");
                #endif
            }
        }
        else if (choice == 2) {
            if (block_count == 0) {
                printf("Error: No blocks allocated\n");
                continue;
            }
            
            printf("Initializing physical memory for last block...\n");
            
            MemoryBlock* block = &blocks[block_count - 1];
            
            if (!HAS_WRITE_PERMISSION(block->permissions)) {
                printf("Error: No write permission\n");
                continue;
            }
            
        
            for (size_t i = 0; i < BLOCK_SIZE; i += PAGE_SIZE) {
                size_t bytes = BLOCK_SIZE - i;
                if (bytes > PAGE_SIZE) bytes = PAGE_SIZE;
                memset((char*)block->address + i, 0xFF, bytes);
            }
            
            if (!block->is_filled) {
                physical_memory += BLOCK_SIZE;
                block->is_filled = 1;
            }
            
            printf("Physical memory initialized!\n");
            printf("Virtual memory: %.1f MB\n", 
                   virtual_memory / (1024.0 * 1024));
            printf("Physical memory: %.1f MB\n", 
                   physical_memory / (1024.0 * 1024));
        }
        else if (choice == 3) { 
            if (block_count == 0) {
                printf("Error: No blocks to free\n");
                continue;
            }
            
            printf("Freeing last block...\n");
            
            MemoryBlock* block = &blocks[block_count - 1];
            
            if (platform_free(block->address, BLOCK_SIZE)) {
                if (block->is_filled) {
                    physical_memory -= BLOCK_SIZE;
                }
                virtual_memory -= BLOCK_SIZE;
                
                block->address = NULL;
                block->is_filled = 0;
                block->permissions = 0;
                block_count--;
                
                printf("Block freed\n");
                printf("Virtual memory: %.1f MB\n", 
                       virtual_memory / (1024.0 * 1024));
                printf("Physical memory: %.1f MB\n", 
                       physical_memory / (1024.0 * 1024));
                printf("Blocks allocated: %d\n", block_count);
            } else {
                printf("Free failed\n");
                #ifdef _WIN32
                    printf("Error code: %lu\n", GetLastError());
                #else
                    perror("munmap");
                #endif
            }
        }
        else if (choice == 4) {  
            printf("Freeing all memory...\n");
            
            for (int i = 0; i < block_count; i++) {
                if (blocks[i].address != NULL) {
                    platform_free(blocks[i].address, BLOCK_SIZE);
                }
            }
            
            printf("All memory returned to system\n");
            printf("Exiting...\n");
            break;
        }
        else {
            printf("Error: Invalid choice\n");
        }
    }
    
    return 0;
}