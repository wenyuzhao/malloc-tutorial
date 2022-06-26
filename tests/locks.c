#include "testing.h"

int main()
{
    void *ptr = mallocing(8);
    freeing(ptr);
    ptr = mallocing(8);
    freeing(ptr);
}
