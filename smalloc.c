#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "smalloc.h"

typedef struct blockHeader blockHeader;

struct blockHeader {
    int block_size; // size of the entire block (header + payload + padding)
    int allocated; // 1 if the block is allocated
    blockHeader* next_free; // pointer to the next free block
    blockHeader* prev_free; // pointer to the previous free block
};

blockHeader* first_block = NULL; // pointer to the start of the heap //change this into a void*
blockHeader* first_free = NULL; // pointer to the first free block

/*
 * my_init() is called one time by the application program to to perform any 
 * necessary initializations, such as allocating the initial heap area.
 * size_of_region is the number of bytes that you should request from the OS using
 * mmap().
 * Note that you need to round up this amount so that you request memory in 
 * units of the page size, which is defined as 4096 Bytes in this project.
 */
int my_init(int size_of_region) {
    // round size up to fit a page 
    int rounded_size = ((size_of_region + 4095) / 4096) * 4096;
    
    // open the file
    int fd = open("/dev/zero", O_RDWR);
    if (fd == -1) {
        perror("Failed to open /dev/zero");
        return -1;
    }

    // map the memory
    void* start_of_heap = mmap(NULL, rounded_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if(start_of_heap == MAP_FAILED){
        close(fd);
        return -1;
    }
    // close the file
    close(fd);

    // initialize the first block 
    first_block = (blockHeader*)start_of_heap;
    first_block ->  block_size = rounded_size;
    first_block -> allocated = 0;
    first_block -> next_free = NULL;
    first_block -> prev_free = NULL;

    // intialize first_free pointer to first block
    first_free = first_block;
    return 0;
}

/*
 * smalloc() takes as input the size in bytes of the payload to be allocated and 
 * returns a pointer to the start of the payload. The function returns NULL if 
 * there is not enough contiguous free space within the memory allocated 
 * by my_init() to satisfy this request.
 */
void *smalloc(int size_of_payload, Malloc_Status *status) {
    //ensures payload is a multiple of 8 bytes for alignment
    int actual_payload_size = (size_of_payload + 7) & ~7; 

    int hops = 0; 
    int fragmentation = 0; // amount of free space remaining
    blockHeader* new_block = NULL; //new block if block needs splitting

    // traverse the allocated memory blocks to find the FIRST free block
    blockHeader* block = first_free;
    
    while(block != NULL){
        // check if the block is free w/enough space
        if(!block -> allocated && block -> block_size >= actual_payload_size + sizeof(blockHeader)){
            fragmentation = block -> block_size - actual_payload_size - sizeof(blockHeader);
            
            // split the block if fragmentation is large enough
            if(fragmentation >= 24){
                // split the block

                // calculate address for the new block header
                new_block = (blockHeader*)((char*)block + actual_payload_size + sizeof(blockHeader));
                
                // update new block header
                new_block -> block_size = fragmentation;
                new_block -> allocated = 0;
                
                // update free list pointers for the new block
                new_block -> prev_free = block -> prev_free;
                new_block -> next_free = block -> next_free; 

                // update size of current block
                block -> block_size = actual_payload_size + sizeof(blockHeader); 
                
                // update current block surronding pointers
                if(block -> prev_free){
                    block -> prev_free -> next_free = new_block;
                }
                if(block -> next_free){
                    block -> next_free -> prev_free = new_block;
                }
                block -> next_free = new_block;
            
            }else{ // fragmentation is less than 24
                if(block -> prev_free){
                    block -> prev_free -> next_free = block -> next_free;
                }
                if(block -> next_free){
                    block -> next_free -> prev_free = block -> prev_free;
                }
            }
            if(hops == 0 && block -> next_free != NULL){
                first_free = block -> next_free;
            }
            // mark block as allocated
            block -> allocated = 1;
            // update malloc status
            status -> hops = hops;
            status -> payload_offset = (unsigned long)block + sizeof(blockHeader) - (unsigned long)first_block; 
            status -> success = 1;
            return block;

        }else{ // move to next block in the free list
            hops += 1;
            block = block -> next_free;
        }
    }
    status -> hops = -1;
    status -> payload_offset = -1;
    status -> success = 0;
    return NULL;
}

/*
 * sfree() frees the target block. "ptr" points to the start of the payload.
 * NOTE: "ptr" points to the start of the payload, rather than the block (header).
 */
void sfree(void *ptr){
    blockHeader* free_block = ptr;

    // if the block's free list is NULL
    if (free_block == NULL) {
        return; // nothing to coalesce with
    }

    // mark the block as free
    free_block -> allocated = 0;
    
    //find prev free and next free of this new free block by iterating though free list
    blockHeader* temp_block = first_free;
    while (temp_block != NULL) {
        if((unsigned long long)temp_block > (unsigned long long)free_block){
            free_block -> next_free = temp_block;
            free_block -> prev_free = temp_block -> prev_free;
            break;
        }
        temp_block = temp_block -> next_free;
    }
    blockHeader* prev = free_block -> prev_free;
    blockHeader* next = free_block -> next_free;
    
    // coalesce adjacent free blocks
    // merge with previous block
    if (prev != NULL && ((unsigned long long)prev + (unsigned long long)prev -> block_size == (unsigned long long)free_block)) {
        prev -> block_size += free_block -> block_size; //increase size of previous block
        prev -> next_free = next; // update next pointer of previous block
        if (next){
            next -> prev_free = prev; // update previous pointer of next block if it exists
        }
        free_block = prev; // update header pointer to point to the merged block
    }

    // merge with next block
    if (next != NULL && ((unsigned long long)next - (unsigned long long)free_block -> block_size == (unsigned long long)free_block)) {
        free_block -> block_size += next -> block_size; // increase size of current block
        free_block -> next_free = next -> next_free; // update next pointer of current block
        if (next -> next_free){
            next -> next_free -> prev_free = free_block; // update previous pointer of next blocks next pointer
        }
        next = free_block;
    }

    // a merge did not happen
    if (next != free_block || free_block != prev){
        if(next == free_block && free_block != prev){
            if(prev){
                prev -> next_free = free_block;
            }
        }
        else if(next != free_block && free_block == prev){
            if(next){
                next -> prev_free = free_block;
            }
        }else{
            //did not merge either side
            if(next){
                next -> prev_free = free_block;
            }
            if(prev){
                prev -> next_free = free_block;
            }
            
        }
        
    }

    // reinitialize first free block
    if (free_block->prev_free == NULL) {
        // the deallocated block is the new head of the free list
        first_free = free_block;
    }
}
