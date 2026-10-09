#include <stdio.h>
#include <stdlib.h>

int main() {
    void *p1 = malloc(256);
    void *p2 = malloc(8);
    
    printf("p1: %p\n", p1);
    printf("p2: %p\n", p2);
    printf("diff: %ld bytes\n", (char*)p2 - (char*)p1);
    
    return 0;
}
