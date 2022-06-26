#include "testing.h"

int main()
{
    void *ptr = mallocing(8);
    mallocing(42);
    freeing(ptr);
}
