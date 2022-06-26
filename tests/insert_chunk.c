#include <unistd.h>
#include "testing.h"

int main()
{
    mallocing(max_allocation_size() - 48);
    // printf("Calling sbrk to allocate memory between malloc's chunks\n");
    sbrk(1024);
    mallocing(max_allocation_size() - 48);
}
