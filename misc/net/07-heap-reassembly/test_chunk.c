#include <stdio.h>
#include <stdlib.h>

struct malloc_chunk {
    size_t      mchunk_prev_size;  // Size of previous chunk (if free).
    size_t      mchunk_size;       // Size in bytes, including overhead. 

    struct malloc_chunk* fd;       // double links, used only if free
    struct malloc_chunk* bk;

    // only used for large blocks: pointer to next larger size
    struct malloc_chunk* fd_nextsize; // double links, used only if free
    struct malloc_chunk* bk_nextsize;
};

int main() {
    void *p1 = malloc(256);
    
    struct malloc_chunk *chunk = (struct malloc_chunk *)((char *)p1 - 16);
    
    printf("user data pointer: %p\n", p1);
    printf("chunk metadata pointer: %p\n", chunk);
    printf("chunk size (raw): 0x%zx\n", chunk->mchunk_size);
    printf("chunk size (actual): %zu bytes\n", chunk->mchunk_size & ~0x7); // Mask out the flag bits
    
    return 0;
}
