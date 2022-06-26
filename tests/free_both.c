#include "testing.h"

int main()
{
    void *ptr = mallocing(8);
    void *ptr2 = mallocing(8);
    freeing(ptr);
    freeing(ptr2);
}
