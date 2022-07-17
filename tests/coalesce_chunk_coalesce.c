#include "testing.h"

int main()
{
    void *ptr = mallocing(8);
    mallocing(kMaxAllocationSize - 128);
    freeing(ptr);
    mallocing(kMaxAllocationSize - 128);
}
