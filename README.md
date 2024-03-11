# Strange Memory Allocator

## Overview
Welcome to the Strange Memory Allocator project! This project aims to implement a unique memory allocator for user-level processes' heap management. While similar to standard memory allocators in functionality, this allocator diverges in design, offering a simplified approach at the expense of memory optimization. This "strange" allocator prioritizes simplicity over efficiency.

## Key Features
- **Simplified Design**: The allocator follows a straightforward approach, focusing on ease of understanding and implementation.
- **Explicit Free List**: Utilizes a doubly-linked list to manage free memory blocks efficiently.
- **Aligned Memory**: Ensures memory allocations are 8-byte aligned, adhering to system requirements.
- **Coalescing**: Implements coalescing to merge adjacent free memory blocks, optimizing memory usage.

## Functionality
The allocator provides the following functions:
1. **`my_init(int size_of_region)`:** Initializes the memory allocator, allocating the initial heap area using `mmap`.
2. **`smalloc(int size_of_payload, Malloc_Status *status)`:** Allocates memory for a payload of a given size, managing fragmentation and updating allocation status.
3. **`sfree(void *ptr)`:** Frees the memory block pointed to by `ptr`, coalescing adjacent free blocks to optimize memory usage.

## Data Structures
- **Explicit Free List:** Maintains a doubly-linked list of free memory blocks, ordered by address.
- **Block Header:** Each block includes a header containing metadata such as block size and allocation status.