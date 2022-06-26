#include "testing.h"

int main()
{
    void *ptr = mallocing(3);
    void *ptr2 = mallocing(2);
    mallocing(1);
    freeing(ptr);
    freeing(ptr2);
}
