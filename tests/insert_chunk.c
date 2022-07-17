#include "testing.h"
#include <sys/mman.h>
#include <unistd.h>

int main()
{
    mallocing(max_allocation_size() - 48);
    // printf("Calling sbrk to allocate memory between malloc's chunks\n");
    void * ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    USE(ptr);
    mallocing(max_allocation_size() - 48);
}
