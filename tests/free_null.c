#include "testing.h"

int main()
{
    freeing(NULL);
    void *ptr = mallocing(8);
    freeing(ptr);
}
