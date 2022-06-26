#include "testing.h"

int main()
{
    void *ptr = mallocing(8);
    mallocing(max_allocation_size() - 128);
    freeing(ptr);
    mallocing(max_allocation_size() - 128);
}
